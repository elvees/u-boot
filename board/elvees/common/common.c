// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021-2024 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <bootstage.h>
#include <display_options.h>
#include <dm.h>
#include <dm/of_access.h>
#include <env.h>
#include <fdt_support.h>
#include <i2c.h>
#include <image.h>
#include <init.h>
#include <mmc.h>
#include <asm/armv8/mmu.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/sections.h>
#include <linux/bitfield.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>

#include <mcom03_sip.h>

#include "mcom03-common.h"

#define mcom03_bootstage_sip(id, param) \
	mcom03_sip_smccc_smc(MCOM03_SIP_BOOTSTAGE, (id), (param), 0, 0, 0, 0, 0)

#define SERVICE_PPOLICY(x)		(0x1F000000UL + (x) * 0x8)
#define SERVICE_PSTATUS(x)		(0x1F000000UL + (x) * 0x8 + 0x4)
#define HSPERIPH_URB_NAND_PADCFG	0x10400184
#define NAND_ENABLE			BIT(0)
#define NAND_V18			BIT(1)
#define NAND_CLE			BIT(2)
#define PAD_MUX_NAND			BIT(4)

#define SERV_URB_TOP_GATECLK		0x1F001008

#define MEDIA_SUBSYSTEM_CFG		0x1322000
#define DISPLAY_PARALLEL_POR_EN		BIT(0)

#define PP_ON				0x10

#define BOOT_TARGET_DEVICES_TRUSTPHONEPM \
	BOOT_TARGET_DEVICES_USB(BOOTENV_DEV_NAME) \
	BOOT_TARGET_DEVICES_MMC(BOOTENV_DEV_NAME) \
	BOOT_TARGET_DEVICES_PXE(BOOTENV_DEV_NAME) \
	""

struct ddrinfo {
	u64 dram_size[CONFIG_DDRMC_MAX_NUMBER];
	u64 total_dram_size;
	struct {
		bool enable;
		int channels;
		int size;
	} interleaving;
	int speed[CONFIG_DDRMC_MAX_NUMBER];
	/* RAM configuration */
	struct {
		u64 start;
		u64 size;
	} mem_regions[CONFIG_NR_DRAM_BANKS];
};

DECLARE_GLOBAL_DATA_PTR;

enum subsystem_reset_lines {
	/* Do not swap elements */
	CPU_SUBS = 0,
	SDR_SUBS,
	MEDIA_SUBS,
	CORE_SUBS,
	HSPERIPH_SUBS,
	LSPERIPH0_SUBS,
	LSPERIPH1_SUBS,
	DDR_SUBS,
	TOP_SUBS,
	RISC0_SUBS,
};

void reset_cpu(void)
{
}

static struct mm_region mcom03_mem_map[] = {
	{
		.virt = PHYS_SDRAM_0,
		.phys = PHYS_SDRAM_0,
		.size = PHYS_SDRAM_0_SIZE,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0x40000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = mcom03_mem_map;

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_0_SIZE;
	return 0;
}

void *board_fdt_blob_setup(int *err)
{
	void *fdt_blob = (void *)CONFIG_MCOM03_EXTERNAL_DTB_ADDR;

	if (fdt_magic(fdt_blob) != FDT_MAGIC) {
		*err = -ENOENT;
		printf("%s: DTB is missing or corrupted at address %p\n",
		       __func__, fdt_blob);
		return NULL;
	}

	*err = 0;
	return fdt_blob;
}

int dram_init_banksize(void)
{
	if (IS_ENABLED(CONFIG_TARGET_HAPS)) {
		gd->bd->bi_dram[0].start = PHYS_SDRAM_0;
		gd->bd->bi_dram[0].size = PHYS_SDRAM_0_SIZE;

		gd->bd->bi_dram[1].start = PHYS_SDRAM_1;
		gd->bd->bi_dram[1].size = PHYS_SDRAM_1_SIZE;
	} else {
		struct ddrinfo *info = (struct ddrinfo *)CONFIG_MEM_REGIONS_ADDR;

		memcpy(gd->bd->bi_dram, info->mem_regions,
		       FIELD_SIZEOF(struct bd_info, bi_dram));
	}

	return 0;
}

static void i2c_pad_cfg(int i2c_num)
{
	u32 val;

	if (i2c_num == 0) {
		/* There are no registers for GPIO0 to enable
		 * the pad receiver */
		writel(0x18, LSP0_GPIO_SWPORTD_CTL);
	} else {
		val = readl(GPIO1_PORTA_PAD_CTR(2 * i2c_num - 2));
		val |= GPIO_PAD_CTR_EN;
		writel(val, GPIO1_PORTA_PAD_CTR(2 * i2c_num - 2));
		val = readl(GPIO1_PORTA_PAD_CTR(2 * i2c_num - 1));
		val |= GPIO_PAD_CTR_EN;
		writel(val, GPIO1_PORTA_PAD_CTR(2 * i2c_num - 1));
		val = readl(LSP1_GPIO_SWPORTA_CTL);
		val |= BIT(2 * i2c_num - 2) | BIT(2 * i2c_num - 1);
		writel(val, LSP1_GPIO_SWPORTA_CTL);
	}
}

static void i2c_enable(void)
{
	for (int i = 0; i < 4; i++) {
		ofnode i2c_node;
		char i2c_alias_name[5];

		sprintf(i2c_alias_name, "i2c%d", i);
		i2c_node = ofnode_get_aliases_node(i2c_alias_name);

		if (ofnode_valid(i2c_node) && ofnode_is_enabled(i2c_node))
			i2c_pad_cfg(i);
	}
}

static void lsperiph1_v18_pad_cfg(void)
{
	u32 val;

	val = readl(LSP1_URB_GPIO1_V18);
	val |= LSP1_URB_GPIO1_V18_V18;
	writel(val, LSP1_URB_GPIO1_V18);
}

static void nand_pad_cfg(void)
{
	// temporary code until NAND support is added to pinctrl
	u32 val = PAD_MUX_NAND | NAND_CLE | NAND_ENABLE;

	if (of_machine_is_compatible("elvees,ecam03bl") ||
	    of_machine_is_compatible("elvees,ecam03dm"))
		val |= NAND_V18;

	writel(val, HSPERIPH_URB_NAND_PADCFG);
}

static void hsp_emac_pad_set_ctl(unsigned long reg, u32 value)
{
	u32 val = readl(reg);

	val &= ~HSP_URB_EMAC_PAD_CTR_CTL;
	val |= FIELD_PREP(HSP_URB_EMAC_PAD_CTR_CTL, value);
	writel(val, reg);
}

static void board_pads_cfg(void)
{
	u32 val;

	if (of_machine_is_compatible("elvees,mcom03bub")) {
		nand_pad_cfg();

		/* Set EMAC pads drive strength to 12 mA for data and 8 mA for clock.
		 * Required for correct operation at 125 MHz 3.3V. See #MCOM03SW-823.
		 */
		hsp_emac_pad_set_ctl(HSP_URB_EMAC0_TX_PADCFG, 0x3f);
		hsp_emac_pad_set_ctl(HSP_URB_EMAC0_TXC_PADCFG, 0xf);
		hsp_emac_pad_set_ctl(HSP_URB_EMAC1_TX_PADCFG, 0x3f);
		hsp_emac_pad_set_ctl(HSP_URB_EMAC1_TXC_PADCFG, 0xf);
	} else if (of_machine_is_compatible("elvees,ecam03bl") ||
		   of_machine_is_compatible("elvees,ecam03dm")) {
		nand_pad_cfg();

		/* U-Boot doesn't have pinctrl driver, so switch pad voltage manually */
		lsperiph1_v18_pad_cfg();
	} else if (of_machine_is_compatible("elvees,iqcam")) {
		/* Set lens motors GPIO pins to logical one output mode in order to prevent
		 * motors overheating caused by bug #IPCAM-740.
		 */
		/* Setup MOT_I_IN1..4 pins */
		val = readl(LSP0_GPIO_SWPORTC_DDR);
		val |= BIT(4) | BIT(2) | BIT(1) | BIT(0);
		writel(val, LSP0_GPIO_SWPORTC_DDR);
		val = readl(LSP0_GPIO_SWPORTC_DR);
		val |= BIT(4) | BIT(2) | BIT(1) | BIT(0);
		writel(val, LSP0_GPIO_SWPORTC_DR);

		/* Setup ICR_ON/OFF pins */
		val = readl(LSP0_GPIO_SWPORTD_DDR);
		val |= BIT(1) | BIT(0);
		writel(val, LSP0_GPIO_SWPORTD_DDR);
		val = readl(LSP0_GPIO_SWPORTD_DR);
		val |= BIT(1) | BIT(0);
		writel(val, LSP0_GPIO_SWPORTD_DR);

		/* U-Boot doesn't have pinctrl driver, so switch pad voltage manually */
		lsperiph1_v18_pad_cfg();

		/* Setup MOT_F_IN1..4, MOT_Z_IN1..3 pins */
		val = readl(LSP1_GPIO_SWPORTC_DDR);
		val |= BIT(7) | BIT(5) | BIT(4) |
		       BIT(3) | BIT(2) | BIT(1) | BIT(0);
		writel(val, LSP1_GPIO_SWPORTC_DDR);
		val = readl(LSP1_GPIO_SWPORTC_DR);
		val |= BIT(7) | BIT(5) | BIT(4) |
		       BIT(3) | BIT(2) | BIT(1) | BIT(0);
		writel(val, LSP1_GPIO_SWPORTC_DR);

		/* Setup MOT_Z_IN4 pin */
		val = readl(LSP1_GPIO_SWPORTD_DDR);
		val |= BIT(2);
		writel(val, LSP1_GPIO_SWPORTD_DDR);
		val = readl(LSP1_GPIO_SWPORTD_DR);
		val |= BIT(2);
		writel(val, LSP1_GPIO_SWPORTD_DR);
	} else if (of_machine_is_compatible("elvees,pm03cam-r2.0")) {
		/* Set lens motors GPIO pins to logical one output mode in order to prevent
		 * motors overheating caused by bug #IPCAM-740.
		 */
		/* Setup GPIO_IRIS pins */
		val = readl(LSP0_GPIO_SWPORTC_DDR);
		val |= BIT(4) | BIT(2) | BIT(1) | BIT(0);
		writel(val, LSP0_GPIO_SWPORTC_DDR);
		val = readl(LSP0_GPIO_SWPORTC_DR);
		val |= BIT(4) | BIT(2) | BIT(1) | BIT(0);
		writel(val, LSP0_GPIO_SWPORTC_DR);

		/* Setup GPIO_IR_CUT pins */
		val = readl(LSP0_GPIO_SWPORTD_DDR);
		val |= BIT(1) | BIT(0);
		writel(val, LSP0_GPIO_SWPORTD_DDR);
		val = readl(LSP0_GPIO_SWPORTD_DR);
		val |= BIT(1) | BIT(0);
		writel(val, LSP0_GPIO_SWPORTD_DR);

		/* U-Boot doesn't have pinctrl driver, so switch pad voltage manually */
		lsperiph1_v18_pad_cfg();

		/* Setup GPIO_ZOOM, GPIO_FOCUS pins */
		writel(0xFF, LSP1_GPIO_SWPORTC_DDR);
		writel(0xFF, LSP1_GPIO_SWPORTC_DR);
	} else if (of_machine_is_compatible("elvees,ip-ku-m1-r1.0")) {
		/* Set EMAC pads drive strength to 12 mA for data and clock.
		 * Required for correct operation at 25 MHz 3.3V.
		 */
		hsp_emac_pad_set_ctl(HSP_URB_EMAC0_TX_PADCFG, 0x3f);
		hsp_emac_pad_set_ctl(HSP_URB_EMAC0_TXC_PADCFG, 0x3f);
		hsp_emac_pad_set_ctl(HSP_URB_EMAC1_TX_PADCFG, 0x3f);
		hsp_emac_pad_set_ctl(HSP_URB_EMAC1_TXC_PADCFG, 0x3f);
	} else if (!of_machine_is_compatible("elvees,elvmc03q7") &&
		   !of_machine_is_compatible("elvees,elvmc03ce") &&
		   !of_machine_is_compatible("elvees,skifmp")) {
		/* U-Boot doesn't have pinctrl driver, so switch pad voltage manually */
		lsperiph1_v18_pad_cfg();
	}

	i2c_enable();
}

static void power_init_elvmc03smarc_r10(void)
{
	u32 val;

	/* Setup CARRIER_PWR_ON signal on ELV-MC03-SMARC */
	/* TODO: rework to use GPIO driver */
	val = readl(LSP1_GPIO_SWPORTC_DDR);
	val |= BIT(5);
	writel(val, LSP1_GPIO_SWPORTC_DDR);

	val = readl(LSP1_GPIO_SWPORTC_DR);
	val |= BIT(5);
	writel(val, LSP1_GPIO_SWPORTC_DR);

	/* Let carrier standby circuits switch on, 1ms should be enough */
	mdelay(1);

	/* Setup CARRIER_STBY# signal on ELV-MC03-SMARC */
	val = readl(LSP1_GPIO_SWPORTD_DDR);
	val |= BIT(0);
	writel(val, LSP1_GPIO_SWPORTD_DDR);

	val = readl(LSP1_GPIO_SWPORTD_DR);
	val |= BIT(0);
	writel(val, LSP1_GPIO_SWPORTD_DR);

	/* Delay >100ms from CARRIER_PWR_ON to RESET_OUT# signals.
	 * The SMARC specification does not explain why this 100ms is needed.
	 * Perhaps we don't need it at all.
	 */
	mdelay(100);
}

static void power_init_elvmc03smarc_r22(void)
{
	u32 val;

	/* Setup CARRIER_STBY# signal on ELV-MC03-SMARC r2.2*/
	val = readl(LSP1_GPIO_SWPORTC_DDR);
	val |= BIT(5);
	writel(val, LSP1_GPIO_SWPORTC_DDR);

	val = readl(LSP1_GPIO_SWPORTC_DR);
	val |= BIT(5);
	writel(val, LSP1_GPIO_SWPORTC_DR);
}

static void power_init_trustphonepm(void)
{
	u32 val;

	/* LTE module requires a pulse on PWRKEY input pin to turn on.
	 * Pulse duration must be at least 500ms.
	 * TODO: Move it to userspace.
	 */
	/* Reset deassert */
	val = readl(MFBSP1_DIR);
	val |= BIT(7);
	writel(val, MFBSP1_DIR);

	val = readl(MFBSP1_DR);
	val |= BIT(7);
	writel(val, MFBSP1_DR);

	/* PWR OFF */
	val = readl(LSP1_GPIO_SWPORTD_DDR);
	val |= BIT(4);
	writel(val, LSP1_GPIO_SWPORTD_DDR);

	val = readl(LSP1_GPIO_SWPORTD_DR);
	val &= ~BIT(4);
	writel(val, LSP1_GPIO_SWPORTD_DR);

	mdelay(500);

	/* PWR ON */
	val = readl(LSP1_GPIO_SWPORTD_DR);
	val |= BIT(4);
	writel(val, LSP1_GPIO_SWPORTD_DR);
}

static void power_init_pm03cam_osm_r104(void)
{
	struct udevice *udev;
	int ret;

	ret = i2c_get_chip_for_busnum(4, 0x4B, 1, &udev);
	if (ret) {
		printf("%s: Cannot find udev for a bus 0\n", __func__);
		return;
	}

	/* Enable the write access to BUCK1 and BUCK5 registers */
	dm_i2c_reg_write(udev, 0x2F, 0x01);

	/* Set BUCK1 RUN voltage to 0.9V */
	dm_i2c_reg_write(udev, 0x0D, 0x14);

	/* Enable BUCK5 */
	dm_i2c_reg_write(udev, 0x09, 0x03);

	/* Disable the write access to BUCK1 and BUCK5 registers */
	dm_i2c_reg_write(udev, 0x2F, 0x03);

	mdelay(1);
}

int power_init_board(void)
{
	if (of_machine_is_compatible("elvees,elvmc03smarc-r1.0"))
		power_init_elvmc03smarc_r10();
	else if (of_machine_is_compatible("elvees,elvmc03smarc-r2.2.0") ||
		 of_machine_is_compatible("elvees,elvmc03smarc-r2.6.1") ||
		 of_machine_is_compatible("elvees,elvmc03smarc-r2.7.1") ||
		 of_machine_is_compatible("elvees,elvmc03smarc-r2.9.1"))
		power_init_elvmc03smarc_r22();
	else if (of_machine_is_compatible("elvees,trustphonepm"))
		power_init_trustphonepm();

	return 0;
}

static int subsystem_reset_deassert(enum subsystem_reset_lines line)
{
	u32 val;

	writel(PP_ON, SERVICE_PPOLICY(line));
	return readl_poll_timeout(SERVICE_PSTATUS(line), val, val == PP_ON,
				  1000);
}

static int xip_disable(int qspi_num)
{
	u32 val;
	uintptr_t xip_en_req, xip_en_out;

	switch (qspi_num) {
	case 0:
		xip_en_req = SERVICE_URB_XIP_EN_REQ;
		xip_en_out = SERVICE_URB_XIP_EN_OUT;
		break;
	case 1:
		xip_en_req = HSP_URB_XIP_EN_REQ;
		xip_en_out = HSP_URB_XIP_EN_OUT;
		break;
	default:
		printf("%s: Unknown bus: QSPI%d\n", __func__, qspi_num);
		return -ENOENT;
	}

	/* Disable XIP mode */

	val = readl(xip_en_req);
	val &= ~QSPI_XIP_EN;
	writel(val, xip_en_req);

	return readl_poll_timeout(xip_en_out, val, !(val & QSPI_XIP_EN), 100);
}

static int mcom03_subsystem_init(enum subsystem_reset_lines line)
{
	/* Order as in subsystem_reset_lines. -1 means that no gate
	 * for subsystem.
	 */
	const int clkgate_bits[] = {2, 3, 1, -1, 4, 5, 6, 7, 8, 0};

	int ret = subsystem_reset_deassert(line);

	if (ret)
		return ret;

	if (line >= 0 && line < ARRAY_SIZE(clkgate_bits) &&
	    clkgate_bits[line] != -1) {
		u32 val = readl(SERV_URB_TOP_GATECLK);

		val |= BIT(clkgate_bits[line]);
		writel(val, SERV_URB_TOP_GATECLK);
	}

	return 0;
}

int board_init(void)
{
	int ret;

	ret = hsperiph_dma32_bus_init();
	if (ret)
		return ret;

	if (IS_ENABLED(CONFIG_DM_I2C))
		if (of_machine_is_compatible("elvees,pm03camosm-r1.04"))
			power_init_pm03cam_osm_r104();

	ret = mcom03_subsystem_init(MEDIA_SUBS);
	if (ret)
		return ret;

	writel(DISPLAY_PARALLEL_POR_EN, MEDIA_SUBSYSTEM_CFG);

	board_pads_cfg();

	for (int i = 0; i < 2; i++) {
		int ret = xip_disable(i);

		if (ret)
			return ret;
	}

	return ret;
}

int misc_init_r(void)
{
	int ret;
	char board_name[BOARD_NAME_MAX_SIZE] = { };

	if (!IS_ENABLED(CONFIG_ENV_IS_NOWHERE) &&
	    !IS_ENABLED(CONFIG_TARGET_MCOM03_ECAM03_RECOVERY) &&
	    !IS_ENABLED(CONFIG_TARGET_MCOM03R_ECAM03_RECOVERY)) {
		if (!env_get("first_boot_checker")) {
			printf("*** First boot\n");
			env_set_hex("first_boot_checker", 0x0);
			env_save();
		}
	}

	ret = load_factory_settings();
	if (ret)
		return ret;

	ret = detect_board_name(board_name);
	if (ret)
		return ret;

	// Setup RESET_OUT# signal on ELV-MC03-SMARC (except Rock Pi)
	if (of_machine_is_compatible("elvees,elvmc03smarc-r1.0") &&
	    strcmp(board_name, "elvmc03smarc-r1.0-rockpi-n10")) {
		u32 val;

		val = readl(LSP1_GPIO_SWPORTD_DDR);
		val |= BIT(7);
		writel(val, LSP1_GPIO_SWPORTD_DDR);

		val = readl(LSP1_GPIO_SWPORTD_DR);
		val |= BIT(7);
		writel(val, LSP1_GPIO_SWPORTD_DR);
	}

	return do_factory_settings(board_name);
}

int board_late_init(void)
{
	if (of_machine_is_compatible("elvees,trustphonepm"))
		env_set("boot_targets",
			BOOT_TARGET_DEVICES_TRUSTPHONEPM);

	return 0;
}

#if CONFIG_IS_ENABLED(OF_LIBFDT)
void board_prep_linux(struct bootm_headers *images)
{
	int offset;
	int ret;
	int len;
	int pos;
	char buf[DISPLAY_OPTIONS_BANNER_LENGTH] = {0};

	display_options_get_banner(false, buf, sizeof(buf));
	len = strlen(buf);
	if (len) {
		pos = strcspn(buf, "\n");
		buf[pos] = '\0';

		ret = fdt_check_header(images->ft_addr);
		if (ret < 0) {
			log_err("%s: %s\n", __func__, fdt_strerror(ret));
			return;
		}

		/* find or create "/chosen" node. */
		offset = fdt_find_or_add_subnode(images->ft_addr, 0, "chosen");
		if (offset < 0)
			return;

		/* override u-boot version */
		ret = fdt_setprop(images->ft_addr, offset, "u-boot,version", buf, pos);
		if (ret < 0) {
			log_err("Could not set u-boot,version %s\n", fdt_strerror(ret));
			return;
		}
	}

	if (mcom03_bootstage_sip(MCOM03_SIP_BOOTSTAGE_SET_STAGE, BOOTSTAGE_ID_RUN_OS) < 0)
		log_err("Failed to set 'RUN_OS' bootstage timestamp\n");
}
#endif

int board_early_init_f(void)
{
	if (mcom03_bootstage_sip(MCOM03_SIP_BOOTSTAGE_SET_STAGE, BOOTSTAGE_ID_START_UBOOT_F) < 0)
		log_err("Failed to set 'START_UBOOT_F' bootstage timestamp\n");

	return 0;
}
