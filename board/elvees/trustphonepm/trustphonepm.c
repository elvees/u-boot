// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/kernel.h>

#include "../common/mcom03-common.h"

DECLARE_GLOBAL_DATA_PTR;

void board_pads_cfg(void)
{
	int i;

	/* U-Boot don't have pinctrl driver, so switch pad voltage manually */
	lsperiph1_v18_pad_cfg();

	for (i = 0; i < 4; i++)
		i2c_pad_cfg(i);
}