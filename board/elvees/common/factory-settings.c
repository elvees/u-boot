// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023-2024 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <ctype.h>
#include <dm.h>
#include <dm/of_access.h>
#include <env.h>
#include <env_internal.h>
#include <i2c.h>
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
	char *boot_targets;
	char *disable_console;
} factory;

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

	return -ENODEV;
}

static int enable_mmc_wp(int dev_num, int bootpart)
{
	u8 wp;
	int ret;
	struct mmc *mmc;

	if (IS_ENABLED(CONFIG_MCOM03_EMMC_BOOT_WP_DISABLED))
		return 0;

	mmc = find_mmc_device(dev_num);

	ALLOC_CACHE_ALIGN_BUFFER(u8, ext_csd, MMC_MAX_BLOCK_LEN);
	ret = mmc_send_ext_csd(mmc, ext_csd);
	if (ret)
		return ret;

	if (bootpart != MMC_BOOT_0 && bootpart != MMC_BOOT_1)
		return -EPERM;

	wp = ext_csd[EXT_CSD_BOOT_WP_STATUS];
	wp >>= (bootpart << 1);

	switch (wp & 3) {
	case 0:
		return mmc_boot_wp_single_partition(mmc, bootpart);
	case 1:
		log_debug("Boot area %d is already power on protected\n", bootpart);
		break;
	case 2:
		log_debug("Boot area %d is already permanently protected\n", bootpart);
		break;
	default:
		log_err("Boot area %d is in reserved protection state\n", bootpart);
		return -EINVAL;
	}

	return 0;
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
		log_debug("\n   There is no filesystem on device\n");
		return 0;
	}

	if (!fs_exists(filename)) {
		log_debug("\n   The %s file doesn't exist\n", filename);
		return 0;
	}

	// Get file size
	ret = fs_set_blk_dev("mmc", part_str, FS_TYPE_ANY);
	if (!ret)
		ret = fs_size(filename, &size);
	if (ret) {
		log_err("\n   Failed to get size of %s: (%d)\n", filename, ret);
		return ret;
	}

	if (size) {
		// Alloc memory to load entire file
		*data = memalign(ARCH_DMA_MINALIGN, size + 1);
		if (!(*data)) {
			log_err("\n   Failed to alloc memory to load %s file\n", filename);
			return -ENOMEM;
		}
		memset((void *)(*data), 0, size + 1);

		// Load entire file
		ret = fs_set_blk_dev("mmc", part_str, FS_TYPE_ANY);
		if (!ret)
			ret = fs_read(filename, (ulong)(*data), 0, 0, &size);
		if (ret) {
			log_err("\n   Failed to load factory settings from %s: (%d)\n",
				filename, ret);
			free(*data);
			*data = NULL;
			return ret;
		}

		// Make sure that all is OK to return real size
		*data_size = size;
	} else {
		log_debug("\n   The %s file is empty\n", filename);
	}

	return ret;
}

static int get_board_name_from_eeprom(char board_name[])
{
#if defined(CONFIG_CMD_EEPROM)
	int ret = 0;
	char *s_pos;
	ofnode i2c_pm_node;
	const char *compat_str = NULL;
	struct udevice *i2c_bus = NULL;
	struct udevice *i2c_chip = NULL;
	char eeprom_data[EEPROM_BOARD_NAME_MAX_SIZE + 1] = { 0 };

	if (!IS_ENABLED(CONFIG_DM_I2C) || !of_machine_is_compatible("elvees,smarccommoncb"))
		return -ENODEV;

	// Find and setup i2c chip
	i2c_pm_node = ofnode_get_aliases_node("i2c_pm");
	if (!ofnode_valid(i2c_pm_node)) {
		log_err("Failed to find 'i2c_pm' device\n");
		return -ENODEV;
	}

	device_find_global_by_ofnode(i2c_pm_node, &i2c_bus);
	if (!i2c_bus) {
		log_err("Failed to get i2c bus from i2c_pm DT node\n");
		return -ENODEV;
	}

	// I2C EEPROM address length is set to 2 bytes by default
	ret = i2c_get_chip(i2c_bus, I2C_PM_CHIP_ADDR, CONFIG_SYS_I2C_EEPROM_ADDR_LEN, &i2c_chip);
	if (ret) {
		log_err("Failed to find chip %x on bus %s\n", I2C_PM_CHIP_ADDR,
			ofnode_get_name(i2c_pm_node));
		return ret;
	}

	// Get and check data from EEPROM
	ret = dm_i2c_read(i2c_chip, 0, eeprom_data, EEPROM_BOARD_NAME_MAX_SIZE);
	if (ret) {
		log_err("Failed to read carrier board I2C EEPROM\n");
		return ret;
	}

	if (!isprint(eeprom_data[0])) {
		log_err("I2C EEPROM is empty\n");
		return -EINVAL;
	}

	/* Construct board name */
	// Get first "compatible" value from the root node and extract DTB name
	compat_str = ofnode_get_property(ofnode_root(), "compatible", NULL);
	if (!compat_str) {
		log_err("Failed to get compatible from the root DT node\n");
		return -ENODEV;
	}

	// Cut out CB common name and add CB name from I2C EEPROM
	strcpy(board_name, strchr(compat_str, ',') + 1);
	if (board_name[0] == '\0') {
		log_err("Failed to find board name in compatible string\n");
		return -EINVAL;
	}

	s_pos = strstr(board_name, "smarccommoncb");
	if (!s_pos) {
		log_err("Failed to find 'smarccommoncb' in compatible\n");
		return -EINVAL;
	}
	s_pos[0] = '\0';

	if (strlen(board_name) + strlen(eeprom_data) > BOARD_NAME_MAX_SIZE) {
		log_err("Not enough space for carrier board name in buffer\n");
		return -EINVAL;
	}

	strcat(board_name, eeprom_data);

	return 0;
#else
	return -ENODEV;
#endif
}

static int get_board_from_dtb(char board_name[])
{
	// Get first "compatible" value from the root node and extract DTB name
	const char *compat_str = ofnode_get_property(ofnode_root(), "compatible", NULL);

	if (!compat_str)
		return -ENODEV;

	const char *dtb_name = strchr(compat_str, ',') + 1;

	strlcpy(board_name, dtb_name, BOARD_NAME_MAX_SIZE);

	return 0;
}

int detect_board_name(char board_name[])
{
	const char *board_override = NULL;

	board_override = env_get("board_override");
	if (board_override) {
		strlcpy(board_name, board_override, BOARD_NAME_MAX_SIZE);
		log_info("Board name set from board_override: %s\n", board_name);
		return 0;
	} else if (factory.board) {
		strlcpy(board_name, factory.board, BOARD_NAME_MAX_SIZE);
		log_info("Board name set from factory settings: %s\n", board_name);
		return 0;
	} else if (!get_board_name_from_eeprom(board_name)) {
		log_info("Board name set from ID EEPROM: %s\n", board_name);
		return 0;
	} else if (of_machine_is_compatible("elvees,elvmc03smarc-smarccommoncb")) {
		/* ROCK Pi N10 is the only carrier which lacks EEPROM, so if we can't detect SMARC
		   board name from factory settings we assume it's ROCK Pi N10 */
		strcpy(board_name, "elvmc03smarc-r1.0-rockpi-n10");
		log_info("Board name set to: %s\n", board_name);
		return 0;
	} else if (!get_board_from_dtb(board_name)) {
		log_info("Board name set from DTB: %s\n", board_name);
		return 0;
	}

	log_err("Unable to detect board name, this should never happen\n");
	return -EINVAL;
}

int load_factory_settings(void)
{
	int ret = 0;

	char *data = NULL;
	char *saved_env = NULL;

	size_t data_size = 0;
	size_t saved_size = 0;

	memset((void *)&factory, 0, sizeof(factory));

	if (IS_ENABLED(CONFIG_MCOM03_DISABLE_FACTORY))
		return 0;

	factory.dev_num = get_mmc_device_num();
	if (factory.dev_num < 0)
		return 0;

	log_info("Loading factory settings from mmc %d.%d:%d ... ",
		 factory.dev_num, MMC_BOOT_0_HW_PART_NUM, MMC_BOOT_0_DEFAULT_PART);

	ret = get_factory_settings(factory.dev_num, MMC_BOOT_0_HW_PART_NUM,
				   MMC_BOOT_0_DEFAULT_PART, "uboot-factory.env",
				   &data, &data_size);
	if (ret)
		return ret;

	if (!data) {
		log_info("not found\n");
		return 0;
	}

	/* Export existing environment to preserve it from being "contaminated"
	 * by temporary factory variables
	 */
	saved_size = hexport_r(&env_htab, '\n', 0, &saved_env, 0, 0, NULL);
	if (saved_size < 0) {
		log_err("\n   Unable to save current env: (%d)\n", errno);
		ret = -errno;
		goto exit;
	}

	/* Import factory variables */
	if (!himport_r(&env_htab, data, data_size, '\n', H_NOCLEAR,
		       0, 0, NULL)) {
		log_err("\n   Unable to import factory settings: (%d)\n", errno);
		ret = -errno;
		goto exit;
	}

	/* Duplicate factory_wp */
	factory.wp = strdup(env_get("factory_wp"));

	/* Duplicate factory_board */
	factory.board = strdup(env_get("factory_board"));

	/* Duplicate factory_eth0_mac */
	factory.eth0_mac = strdup(env_get("factory_eth0_mac"));

	/* Duplicate factory_eth1_mac */
	factory.eth1_mac = strdup(env_get("factory_eth1_mac"));

	/* Duplicate factory_serial */
	factory.serial = strdup(env_get("factory_serial"));

	/* Duplicate factory_boot_targets */
	factory.boot_targets = strdup(env_get("factory_boot_targets"));

	/* Duplicate factory_disable_console */
	factory.disable_console = strdup(env_get("factory_disable_console"));

	/* Restore saved environment */
	if (!himport_r(&env_htab, saved_env, saved_size, '\n', 0, 0, 0, NULL)) {
		log_err("\n   Unable to restore env: (%d)\n", errno);
		ret = -errno;
		goto exit;
	}

	log_info("OK\n");

exit:
	/* Free allocated resources if necessary */
	if (data)
		free(data);
	if (saved_env)
		free(saved_env);

	return ret;
}

int do_factory_settings(const char *board_name)
{
	int ret = 0;

	if (!board_name) {
		log_err("%s called with the NULL 'board_name' pointer\n", __func__);
		ret = -EINVAL;
		goto exit;
	}

	/* Make the boot0 part of current eMMC to be write protected if necessary */
	if (factory.wp) {
		ret = enable_mmc_wp(factory.dev_num, MMC_BOOT_0);
		if (ret) {
			log_err("Unable to write protect mmc %d boot %d\n",
				factory.dev_num, MMC_BOOT_0);
			goto exit;
		}
	}

	if (env_set("board", board_name)) {
		log_err("Unable to set board using value %s\n", board_name);
		ret = -EINVAL;
		goto exit;
	}

	/* Set ethaddr with factory one if necessary. If ethaddr is not set, it will be set
	 * to random one in case of CONFIG_NET_RANDOM_ETHADDR=y
	 */
	if (!env_get("ethaddr")) {
		if (factory.eth0_mac) {
			if (env_set("ethaddr", factory.eth0_mac)) {
				log_err("Unable to set ethaddr using factory value %s\n",
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
				log_err("Unable to set eth1addr using factory value %s\n",
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
				log_err("Unable to set serial# using factory value %s\n",
					factory.serial);
				ret = -EINVAL;
				goto exit;
			}
		}
	}

	/* Set boot_targets with factory value if necessary */
	if (factory.boot_targets) {
		if (env_set("boot_targets", factory.boot_targets)) {
			log_err("Unable to set boot_targets using factory value %s\n",
				factory.boot_targets);
			ret = -EINVAL;
			goto exit;
		}
	}

	if (IS_ENABLED(CONFIG_DISABLE_CONSOLE)) {
		/* Set disable_console with factory value if necessary */
		if (factory.disable_console) {
			if (env_set("disable_console", factory.disable_console)) {
				log_err("Unable to set disable_console using factory value %s\n",
					factory.disable_console);
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
	if (factory.boot_targets)
		free(factory.boot_targets);
	if (factory.disable_console)
		free(factory.disable_console);

	return ret;
}
