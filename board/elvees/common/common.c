// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <env.h>
#include <init.h>
#include <asm/armv8/mmu.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/bitfield.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>

#include "mcom03-common.h"
#include <asm/arch/mcom03-clk.h>

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

int board_init(void)
{
#ifdef CONFIG_MCOM03_SUBSYSTEM_SDR
	enum subsystem_reset_lines reset_lines[] = { MEDIA_SUBS, SDR_SUBS };
#else
	enum subsystem_reset_lines reset_lines[] = { MEDIA_SUBS };
#endif

	/* Order as in subsystem_reset_lines. -1 means that no gate
	 * for subsystem */
	int clkgate_bits[] = {2, 3, 1, -1, 4, 5, 6, 7, 8, 0};
	u32 val;
	int clkgate_bit;
	int ret;
	int i;

	/* Configure all devices to see DDR High address range,
	 * starting from 0x8_0000_0000.
	 */
	writel(0x20, DDR_SUBS_URB_BASE + HSPERIPH_BAR);
	writel(0x20, DDR_SUBS_URB_BASE + GPU_BAR);
	writel(0x20, DDR_SUBS_URB_BASE + LSPERIPH0_BAR);
	writel(0x20, DDR_SUBS_URB_BASE + LSPERIPH1_BAR);

	for (i = 0; i < ARRAY_SIZE(reset_lines); i++) {
		clkgate_bit = reset_lines[i];
		ret = subsystem_reset_deassert(reset_lines[i]);
		if (ret)
			return ret;
		if (clkgate_bit >= 0) {
			val = readl(SERV_URB_TOP_GATECLK);
			val |= BIT(clkgate_bits[clkgate_bit]);
			writel(val, SERV_URB_TOP_GATECLK);
		}
	}

	writel(DISPLAY_PARALLEL_POR_EN, MEDIA_SUBSYSTEM_CFG);

	board_pads_cfg();

	for (i = 0; i < 2; i++) {
		ret = xip_disable(i);
		if (ret)
			return ret;
	}

	return clk_cfg();
}

#ifdef CONFIG_MISC_INIT_R
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
