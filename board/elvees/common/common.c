// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <asm/armv8/mmu.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>

#include "mcom03-clk.h"

#define DDR_SUBS_URB_BASE		0xC000000
#define HSPERIPH_BAR			0xDC

#define GPIO1_PORTA_PAD_CTR(x)		(0x17e0020 + (x) * 0x4)
#define LSP1_GPIO_SWPORTA_CTL		0x1780008
#define GPIO_PAD_CTR_EN			BIT(12)

#define LSP0_GPIO_SWPORTD_CTL		0x161002c

#define SERVICE_PPOLICY(x)		(0x1F000000 + (unsigned long)(x) * 0x8)
#define SERVICE_PSTATUS(x)		(0x1F000000 + (unsigned long)(x) * 0x8 \
					+ 0x4)
#define SERV_URB_TOP_GATECLK		0x1F001008

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

static int subsystem_reset_deassert(enum subsystem_reset_lines line)
{
	u32 val;

	writel(PP_ON, SERVICE_PPOLICY(line));
	return readl_poll_timeout(SERVICE_PSTATUS(line), val, val == PP_ON,
				  1000);
}

static void setup_pads(void)
{
	u32 val;

	/* Used for I2C1_SCL */
	val = readl(GPIO1_PORTA_PAD_CTR(0));
	val |= GPIO_PAD_CTR_EN;
	writel(val, GPIO1_PORTA_PAD_CTR(0));

	/* Used for I2C1_DAT */
	val = readl(GPIO1_PORTA_PAD_CTR(1));
	val |= GPIO_PAD_CTR_EN;
	writel(val, GPIO1_PORTA_PAD_CTR(1));

	/* Used for I2C2_SCL */
	val = readl(GPIO1_PORTA_PAD_CTR(2));
	val |= GPIO_PAD_CTR_EN;
	writel(val, GPIO1_PORTA_PAD_CTR(2));

	/* Used for I2C2_DAT */
	val = readl(GPIO1_PORTA_PAD_CTR(3));
	val |= GPIO_PAD_CTR_EN;
	writel(val, GPIO1_PORTA_PAD_CTR(3));

	/* Used for I2C3_SCL */
	val = readl(GPIO1_PORTA_PAD_CTR(4));
	val |= GPIO_PAD_CTR_EN;
	writel(val, GPIO1_PORTA_PAD_CTR(4));

	/* Used for I2C3_DAT */
	val = readl(GPIO1_PORTA_PAD_CTR(5));
	val |= GPIO_PAD_CTR_EN;
	writel(val, GPIO1_PORTA_PAD_CTR(5));

	/* Turn GPIO1_A0 ... GPIO1_A5 to hardware mode (I2C1, I2C2, I2C3) */
	writel(0x3f, LSP1_GPIO_SWPORTA_CTL);

	/* Turn GPIO0_D3 ... GPIO0_D4 to hardware mode (I2C0) */
	writel(0x18, LSP0_GPIO_SWPORTD_CTL);
}

int board_init(void)
{
	enum subsystem_reset_lines reset_lines[] = { MEDIA_SUBS };

	/* Order as in subsystem_reset_lines. -1 means that no gate
	 * for subsystem */
	int clkgate_bits[] = {2, 3, 1, -1, 4, 5, 6, 7, 8, 0};
	u32 val;
	int clkgate_bit;
	int ret;
	int i;

	/* DDR high memory range is not available on HAPS. It means that
	 * GEMAC DMA has access to DDR only if HSPERIPH_BAR is set to 0.
	 */
	writel(0, DDR_SUBS_URB_BASE + HSPERIPH_BAR);

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

	setup_pads();

	return clk_cfg();
}
