// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021-2023 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <dm.h>
#include <dm/of_access.h>
#include <env.h>
#include <init.h>
#include <asm/armv8/mmu.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <asm/sections.h>
#include <linux/bitfield.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>

#include <asm/arch/mcom03-clk.h>
#include "mcom03-common.h"

#define DDR_SUBS_URB_BASE		0xC000000
#define HSPERIPH_BAR			0xDC
#define GPU_BAR				0xD8
#define LSPERIPH0_BAR			0xE0
#define LSPERIPH1_BAR			0xE4

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

#define BOOT_TARGET_DEVICES_TRUSTPHONEPM(func) \
	BOOT_TARGET_DEVICES_USB(func) \
	BOOT_TARGET_DEVICES_MMC(func) \
	BOOT_TARGET_DEVICES_PXE(func)

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
		return NULL;
	}

	*err = 0;
	return fdt_blob;
}

/* dram_init_banksize() for HAPS placed in board/elvees/haps/haps.c */
#ifndef CONFIG_TARGET_HAPS
int dram_init_banksize(void)
{
	struct ddrinfo *info = (struct ddrinfo *)CONFIG_MEM_REGIONS_ADDR;

	memcpy(gd->bd->bi_dram, info->mem_regions,
	       FIELD_SIZEOF(struct bd_info, bi_dram));

	return 0;
}
#endif

void board_pads_cfg(void)
{
	if (of_machine_is_compatible("elvees,mcom03-bub")) {
		nand_pad_cfg();

		/* Set EMAC pads drive strength to 12 mA for data and 8 mA for clock.
		 * Required for correct operation at 125 MHz 3.3V. See #MCOM03SW-823 */
		pad_set_ctl(HSP_URB_EMAC0_TX_PADCFG, 0x3f);
		pad_set_ctl(HSP_URB_EMAC0_TXC_PADCFG, 0xf);
		pad_set_ctl(HSP_URB_EMAC1_TX_PADCFG, 0x3f);
		pad_set_ctl(HSP_URB_EMAC1_TXC_PADCFG, 0xf);

		for (int i = 0; i < 4; i++)
			i2c_pad_cfg(i);
	} else if (of_machine_is_compatible("elvees,ecam03bl") ||
		   of_machine_is_compatible("elvees,ecam03dm")) {
		nand_pad_cfg();

		/* U-Boot doesn't have pinctrl driver, so switch pad voltage manually */
		lsperiph1_v18_pad_cfg();
	} else {
		/* U-Boot doesn't have pinctrl driver, so switch pad voltage manually */
		lsperiph1_v18_pad_cfg();
	}
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
	 * Perhaps we don't need it at all. */
	mdelay(100);

	if (of_machine_is_compatible("radxa,rockpi-n10"))
		return;

	/* Setup RESET_OUT# signal on ELV-MC03-SMARC */
	val = readl(LSP1_GPIO_SWPORTD_DDR);
	val |= BIT(7);
	writel(val, LSP1_GPIO_SWPORTD_DDR);

	val = readl(LSP1_GPIO_SWPORTD_DR);
	val |= BIT(7);
	writel(val, LSP1_GPIO_SWPORTD_DR);
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
	 * TODO: Move it to userspace. */
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

int power_init_board(void)
{
	if (of_machine_is_compatible("elvees,elvmc03smarc-r1.0"))
		power_init_elvmc03smarc_r10();
	else if (of_machine_is_compatible("elvees,elvmc03smarc-r2.2"))
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

static void pad_set_bits(unsigned long reg, u32 field, u32 value)
{
	u32 val = readl(reg);

	val &= ~field;
	val |= FIELD_PREP(field, value);
	writel(val, reg);
}

void pad_set_ctl(unsigned long reg, u32 value)
{
	pad_set_bits(reg, LSP1_URB_GPIO1_PAD_CTR_CTL, value);
}

void lsperiph1_v18_pad_cfg(void)
{
	u32 val;

	val = readl(LSP1_URB_GPIO1_V18);
	val |= LSP1_URB_GPIO1_V18_V18;
	writel(val, LSP1_URB_GPIO1_V18);
}

void i2c_pad_cfg(int i2c_num)
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

void nand_pad_cfg(void)
{
	// temporary code until NAND support is added to pinctrl
	u32 val = PAD_MUX_NAND | NAND_CLE | NAND_ENABLE;

	if (IS_ENABLED(CONFIG_TARGET_ECAM03DM) ||
	    IS_ENABLED(CONFIG_TARGET_ECAM03BL))
		val |= NAND_V18;

	writel(val, HSPERIPH_URB_NAND_PADCFG);
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
	 * for subsystem */
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

static inline bool is_sdr_enabled(void)
{
	ofnode sdr_node = ofnode_path("/sdr@1900000");

	return ofnode_valid(sdr_node);
}

int board_init(void)
{
	int ret;

	/* Configure all devices to see DDR High address range,
	 * starting from 0x8_0000_0000.
	 */
	writel(0x20, DDR_SUBS_URB_BASE + HSPERIPH_BAR);
	writel(0x20, DDR_SUBS_URB_BASE + GPU_BAR);
	writel(0x20, DDR_SUBS_URB_BASE + LSPERIPH0_BAR);
	writel(0x20, DDR_SUBS_URB_BASE + LSPERIPH1_BAR);

	ret = mcom03_subsystem_init(MEDIA_SUBS);
	if (ret)
		return ret;

	if (is_sdr_enabled()) {
		ret = mcom03_subsystem_init(SDR_SUBS);
		if (ret)
			return ret;
	}

	writel(DISPLAY_PARALLEL_POR_EN, MEDIA_SUBSYSTEM_CFG);

	board_pads_cfg();

	for (int i = 0; i < 2; i++) {
		int ret = xip_disable(i);

		if (ret)
			return ret;
	}

	ret = clk_cfg();
	if (ret)
		return ret;

	if (is_sdr_enabled())
		ret = clk_cfg_sdr();

	return ret;
}

#if IS_ENABLED(CONFIG_MISC_INIT_R)
int misc_init_r(void)
{
	if (!IS_ENABLED(CONFIG_ENV_IS_NOWHERE))
		if (!env_get("first_boot_checker")) {
			printf("*** First boot\n");
			env_set_hex("first_boot_checker", 0x0);
			env_save();
		}

	return 0;
}
#endif

#if IS_ENABLED(CONFIG_BOARD_LATE_INIT)
int board_late_init(void)
{
	if (of_machine_is_compatible("elvees,trustphonepm"))
		env_set("boot_targets",
			BOOT_TARGET_DEVICES_TRUSTPHONEPM(BOOTENV_DEV_NAME));

	/* The following code gets first "compatible" value from the root node,
	 * extracts DTB name and sets it to "board" environment variable for
	 * dynamic DTB selection during Linux booting */
	int compat_strlen;
	const char *compat_str = ofnode_get_property(ofnode_root(), "compatible",
						     &compat_strlen);

	if (!compat_str || !compat_strlen)
		return -ENODEV;

	const char *dtb_name = strchr(compat_str, ',') + 1;

	env_set("board", dtb_name);

	return 0;
}
#endif
