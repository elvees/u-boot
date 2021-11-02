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
	int i;

	nand_pad_cfg();

	// Enable pads for EMAC0/EMAC1 interrupt
	pad_set_e(LSP1_URB_GPIO1_PAD_CTR_ADDR(GPIO_PORTA, 6), 1);
	pad_set_e(LSP1_URB_GPIO1_PAD_CTR_ADDR(GPIO_PORTA, 7), 1);
	for (i = 0; i < 4; i++)
		i2c_pad_cfg(i);
}
