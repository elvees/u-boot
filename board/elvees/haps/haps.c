// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <asm/armv8/mmu.h>
#include <asm/io.h>
#include <asm/sections.h>
#include <linux/kernel.h>

#define DDR_SUBS_URB_BASE		0xC000000
#define HSPERIPH_BAR			0xDC

DECLARE_GLOBAL_DATA_PTR;

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

int board_init(void)
{
	/* DDR high memory range is not available on HAPS. It means that
	 * GEMAC DMA has access to DDR only if HSPERIPH_BAR is set to 0.
	 */
	writel(0, DDR_SUBS_URB_BASE + HSPERIPH_BAR);

	return 0;
}

int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_0;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_0_SIZE;

	gd->bd->bi_dram[1].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[1].size = PHYS_SDRAM_1_SIZE;

	return 0;
}

/* If external DTB is passed to U-Boot, use it. Otherwise use
 * DTB appended to U-Boot image (default U-Boot behavior).
 */
void *board_fdt_blob_setup(void)
{
	void *fdt_blob = NULL;

	if (fdt_magic(CONFIG_MCOM03_EXTERNAL_DTB_ADDR) == FDT_MAGIC)
		fdt_blob = (ulong *)CONFIG_MCOM03_EXTERNAL_DTB_ADDR;
	else
		fdt_blob = (ulong *)&_end;

	return fdt_blob;
}
