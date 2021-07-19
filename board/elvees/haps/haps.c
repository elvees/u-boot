// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 - 2021 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <asm/io.h>
#include <linux/kernel.h>
#include <asm/sections.h>

DECLARE_GLOBAL_DATA_PTR;

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
