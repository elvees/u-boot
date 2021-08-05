// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <asm/io.h>
#include <linux/kernel.h>

DECLARE_GLOBAL_DATA_PTR;

int dram_init_banksize(void)
{
	memcpy(gd->bd->bi_dram, (void *)CONFIG_MEM_REGIONS_ADDR,
	       sizeof(struct bd_info));

	return 0;
}
