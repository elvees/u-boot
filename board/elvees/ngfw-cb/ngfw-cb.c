// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <asm/io.h>
#include <linux/kernel.h>

#include "../common/mcom03-common.h"

DECLARE_GLOBAL_DATA_PTR;

int dram_init_banksize(void)
{
	memcpy(gd->bd->bi_dram, (void *)CONFIG_MEM_REGIONS_ADDR,
	       sizeof(struct bd_info));

	return 0;
}

void board_pads_cfg(void)
{
	int i;
	u32 val;

	for (i = 0; i < 4; i++)
		i2c_pad_cfg(i);

	/* Enable receivers for GPIO1_A6 and GPIO1_D7,
	 * which are used on ELV_MC03_SMARC as HDMI_INT and RESET_OUT.
	 */
	val = readl(GPIO1_PORTA_PAD_CTR(6));
	val |= GPIO_PAD_CTR_EN;
	writel(val, GPIO1_PORTA_PAD_CTR(6));

	val = readl(GPIO1_PORTD_PAD_CTR(7));
	val |= GPIO_PAD_CTR_EN;
	writel(val, GPIO1_PORTD_PAD_CTR(7));
}
