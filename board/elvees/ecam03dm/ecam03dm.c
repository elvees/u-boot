// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <asm/io.h>
#include <linux/kernel.h>

#include "../common/mcom03-common.h"

DECLARE_GLOBAL_DATA_PTR;

void board_pads_cfg(void)
{
	u32 val;
	nand_pad_cfg();

	/* U-Boot doesn't have pinctrl driver, so switch pad voltage manually */
	lsperiph1_v18_pad_cfg();

	/* Enable receivers for GPIO1_A6,
	 * which are used as Ethernet PHY interrupt.
	 * Temporary code until support is added to pinctrl
	 */
	val = readl(GPIO1_PORTA_PAD_CTR(6));
	val |= GPIO_PAD_CTR_EN;
	writel(val, GPIO1_PORTA_PAD_CTR(6));
}
