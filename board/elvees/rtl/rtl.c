// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2019 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/kernel.h>
#include <asm/armv8/mmu.h>
#include <cpu_func.h>

DECLARE_GLOBAL_DATA_PTR;

void reset_cpu(void)
{
}

static struct mm_region rtl_mem_map[] = {
	{
		.virt = CONFIG_SYS_SDRAM_BASE,
		.phys = PHYS_SDRAM_0,
		.size = PHYS_SDRAM_0_SIZE,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0x1640000UL,
		.phys = 0x1640000UL,
		.size = 0x1000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = rtl_mem_map;

void enable_caches(void)
{
	/* Data cache is not enabled here to reduce RTL simulation time */
	icache_enable();
}

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_0_SIZE;
	return 0;
}

int board_init(void)
{
	return 0;
}
