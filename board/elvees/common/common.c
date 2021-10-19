// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <asm/armv8/mmu.h>
#include <asm/io.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>

#include "mcom03-common.h"
#include "mcom03-clk.h"

#define DDR_SUBS_URB_BASE		0xC000000
#define HSPERIPH_BAR			0xDC
#define GPU_BAR				0xD8

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

void reset_cpu(ulong addr)
{
}

static struct mm_region haps_mem_map[] = {
	{
		.virt = CONFIG_SYS_SDRAM_BASE,
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

struct mm_region *mem_map = haps_mem_map;

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_0_SIZE;
	return 0;
}

int dram_init_banksize(void)
{
	memcpy(gd->bd->bi_dram, (void *)CONFIG_MEM_REGIONS_ADDR,
	       sizeof(struct bd_info));

	return 0;
}

static int subsystem_reset_deassert(enum subsystem_reset_lines line)
{
	u32 val;

	writel(PP_ON, SERVICE_PPOLICY(line));
	return readl_poll_timeout(SERVICE_PSTATUS(line), val, val == PP_ON,
				  1000);
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

	if (IS_ENABLED(CONFIG_TARGET_ECAM03DM))
		val |= NAND_V18;

	writel(val, HSPERIPH_URB_NAND_PADCFG);
}

int board_init(void)
{
	enum subsystem_reset_lines reset_lines[] = { MEDIA_SUBS, SDR_SUBS };

	/* Order as in subsystem_reset_lines. -1 means that no gate
	 * for subsystem */
	int clkgate_bits[] = {2, 3, 1, -1, 4, 5, 6, 7, 8, 0};
	u32 val;
	int clkgate_bit;
	int ret;
	int i;

	/* Configure all devices to see DDR Low range (see #MCOM03BUP-234) */
	writel(0, DDR_SUBS_URB_BASE + HSPERIPH_BAR);
	writel(0, DDR_SUBS_URB_BASE + GPU_BAR);

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

	return clk_cfg();
}
