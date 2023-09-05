// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <dm.h>
#include <dm/of_access.h>
#include <env.h>
#include <env_internal.h>
#include <mmc.h>
#include <fs.h>
#include <malloc.h>
#include <memalign.h>
#include "mcom03-common.h"

#define MMC_USER_HW_PART_NUM		0
#define MMC_BOOT_0_HW_PART_NUM		1
#define MMC_BOOT_1_HW_PART_NUM		2
#define MMC_RPMB_HW_PART_NUM		3

#define MMC_BOOT_0			0
#define MMC_BOOT_1			1

#define MMC_BOOT_0_DEFAULT_PART		0

struct factory_settings {
	int dev_num;
	char *wp;
	char *board;
	char *eth0_mac;
	char *eth1_mac;
	char *serial;
};

static int set_board_from_dtb(void)
{
	int compat_strlen;
	/* Get first "compatible" value from the root node and extract
	 * DTB name */
	const char *compat_str = ofnode_get_property(ofnode_root(),
						     "compatible",
						     &compat_strlen);

	if (!compat_str || !compat_strlen)
		return -ENODEV;

	const char *dtb_name = strchr(compat_str, ',') + 1;

	if (env_set("board", dtb_name))
		return -EINVAL;

	return 0;
}

static int get_mmc_device_num(void)
{
	int i;
	struct mmc *mmc;
	int count = get_mmc_num();

	for (i = 0; i < count; ++i) {
		mmc = find_mmc_device(i);
		if (mmc && !mmc_init(mmc) &&
		    !IS_SD(mmc) && mmc->version >= MMC_VERSION_4_5)
			return i;
	}

	printf("eMMC is not found\n");

	return -ENODEV;
}

static int enable_mmc_wp(int dev_num, int bootpart)
{
	if (IS_ENABLED(CONFIG_MCOM03_EMMC_BOOT_WP_DISABLED))
		return 0;

	return mmc_boot_wp_single_partition(find_mmc_device(dev_num), bootpart);
}

static int get_factory_settings(int dev_num, int hwpart, int part, const char *filename,
				char **data, size_t *data_size)
{
	int ret = 0;
	char part_str[6];
	loff_t size;

	if (!filename || !data || !data_size)
		return -EINVAL;

	*data = NULL;
	*data_size = 0;

	snprintf(part_str, sizeof(part_str), "%d.%d:%d", dev_num, hwpart, part);

	// Check that file exists
	ret = fs_set_blk_dev("mmc", part_str, FS_TYPE_ANY);
	if (ret) {
		printf("\n   Failed to find any filesystem on device\n");
		return 0;
	}

	if (!fs_exists(filename)) {
		printf("\n   File %s doesn't exist\n", filename);
		return 0;
	}

	// Get file size
	ret = fs_set_blk_dev("mmc", part_str, FS_TYPE_ANY);
	if (!ret)
		ret = fs_size(filename, &size);
	if (ret) {
		printf("\n   Failed to get size of %s: (%d)\n", filename, ret);
		return ret;
	}

	if (size) {
		// Alloc memory to load entire file
		*data = memalign(ARCH_DMA_MINALIGN, size + 1);
		if (!(*data)) {
			printf("\n   Failed to alloc memory to load factory settings\n");
			return -ENOMEM;
		}
		memset((void *)(*data), 0, size + 1);

		// Load entire file
		ret = fs_set_blk_dev("mmc", part_str, FS_TYPE_ANY);
		if (!ret)
			ret = fs_read(filename, (ulong)(*data), 0, 0, &size);
		if (ret) {
			printf("\n   Failed to load factory settings from %s: (%d)\n",
			       filename, ret);
			free(*data);
			*data = NULL;
			return ret;
		}

		// Make sure that all is OK to return real size
		*data_size = size;
	} else {
		printf("\n   Factory settings are empty\n");
	}

	return ret;
}

static int load_factory_settings(struct factory_settings *factory)
{
	int ret = 0;

	char *data = NULL;
	char *saved_env = NULL;

	size_t data_size = 0;
	size_t saved_size = 0;

	if (!factory)
		return -EINVAL;

	factory->dev_num = get_mmc_device_num();
	if (factory->dev_num < 0)
		return 0;

	printf("Loading factory settings from mmc %d.%d:%d ... ",
	       factory->dev_num, MMC_BOOT_0_HW_PART_NUM, MMC_BOOT_0_DEFAULT_PART);

	ret = get_factory_settings(factory->dev_num, MMC_BOOT_0_HW_PART_NUM,
				   MMC_BOOT_0_DEFAULT_PART, "uboot-factory.env",
				   &data, &data_size);
	if (ret)
		return ret;

	if (!data)
		return 0;

	/* Export existing environment to preserve it from being "contaminated"
	 * by temporary factory variables
	 */
	saved_size = hexport_r(&env_htab, '\n', 0, &saved_env, 0, 0, NULL);
	if (saved_size < 0) {
		printf("\n   Unable to save current env: (%d)\n", errno);
		ret = -errno;
		goto exit;
	}

	/* Import factory variables */
	if (!himport_r(&env_htab, data, data_size, '\n', H_NOCLEAR,
		       0, 0, NULL)) {
		printf("\n   Unable to import factory settings: (%d)\n", errno);
		ret = -errno;
		goto exit;
	}

	/* Duplicate factory_wp */
	factory->wp = strdup(env_get("factory_wp"));

	/* Duplicate factory_board */
	factory->board = strdup(env_get("factory_board"));

	/* Duplicate factory_eth0_mac */
	factory->eth0_mac = strdup(env_get("factory_eth0_mac"));

	/* Duplicate factory_eth1_mac */
	factory->eth1_mac = strdup(env_get("factory_eth1_mac"));

	/* Duplicate factory_serial */
	factory->serial = strdup(env_get("factory_serial"));

	/* Restore saved environment */
	if (!himport_r(&env_htab, saved_env, saved_size, '\n', 0, 0, 0, NULL)) {
		printf("\n   Unable to restore env: (%d)\n", errno);
		ret = -errno;
		goto exit;
	}

	printf("OK\n");

exit:
	/* Free allocated resources if necessary */
	if (data)
		free(data);
	if (saved_env)
		free(saved_env);

	return ret;
}

int do_factory_settings(void)
{
	int ret = 0;

	char *board_override = NULL;

	struct factory_settings factory;

	memset(&factory, 0, sizeof(struct factory_settings));

	if (!IS_ENABLED(CONFIG_MCOM03_DISABLE_FACTORY)) {
		ret = load_factory_settings(&factory);
		if (ret)
			goto exit;
	}

	/* Make the boot0 part of current eMMC to be write protected if necessary */
	if (factory.wp) {
		ret = enable_mmc_wp(factory.dev_num, MMC_BOOT_0);
		if (ret) {
			printf("\n   Unable to write protect mmc %d boot %d\n",
			       factory.dev_num, MMC_BOOT_0);
			goto exit;
		}
	}

	/* Override board or set with factory value or get from dtb if necessary */
	board_override = env_get("board_override");
	if (board_override) {
		if (env_set("board", board_override)) {
			printf("\n   Unable to override board using value %s\n",
			       board_override);
			ret = -EINVAL;
			goto exit;
		}
	} else if (factory.board) {
		if (env_set("board", factory.board)) {
			printf("\n   Unable to set board using factory value %s\n",
			       factory.board);
			ret = -EINVAL;
			goto exit;
		}
	} else {
		ret = set_board_from_dtb();
		if (ret) {
			printf("\n   Unable to set board using dtb value\n");
			goto exit;
		}
	}

	/* Set ethaddr with factory one if necessary. If ethaddr is not set, it will be set
	 * to random one in case of CONFIG_NET_RANDOM_ETHADDR=y
	 */
	if (!env_get("ethaddr")) {
		if (factory.eth0_mac) {
			if (env_set("ethaddr", factory.eth0_mac)) {
				printf("\n   Unable to set ethaddr using factory value %s\n",
				       factory.eth0_mac);
				ret = -EINVAL;
				goto exit;
			}
		}
	}

	/* Set eth1addr with factory one if necessary. If eth1addr is not set, it will be set
	 * to random one in case of CONFIG_NET_RANDOM_ETHADDR=y
	 */
	if (!env_get("eth1addr")) {
		if (factory.eth1_mac) {
			if (env_set("eth1addr", factory.eth1_mac)) {
				printf("\n   Unable to set eth1addr using factory value %s\n",
				       factory.eth1_mac);
				ret = -EINVAL;
				goto exit;
			}
		}
	}

	/* Set serial# with factory value if necessary */
	if (!env_get("serial#")) {
		if (factory.serial) {
			if (env_set("serial#", factory.serial)) {
				printf("\n   Unable to set serial# using factory value %s\n",
				       factory.serial);
				ret = -EINVAL;
				goto exit;
			}
		}
	}

exit:
	/* Free allocated resources if necessary */
	if (factory.wp)
		free(factory.wp);
	if (factory.board)
		free(factory.board);
	if (factory.eth0_mac)
		free(factory.eth0_mac);
	if (factory.eth1_mac)
		free(factory.eth1_mac);
	if (factory.serial)
		free(factory.serial);

	return ret;
}
