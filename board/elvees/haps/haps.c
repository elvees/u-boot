// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019-2023 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/kernel.h>
#include <asm/sections.h>

#include "../common/mcom03-common.h"

DECLARE_GLOBAL_DATA_PTR;

int dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_0;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_0_SIZE;

	gd->bd->bi_dram[1].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[1].size = PHYS_SDRAM_1_SIZE;

	return 0;
}
