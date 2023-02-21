// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2022 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <asm/global_data.h>

#include "../common/mcom03-common.h"

DECLARE_GLOBAL_DATA_PTR;

void board_pads_cfg(void)
{
	/* U-Boot don't have pinctrl driver, so switch pad voltage manually */
	lsperiph1_v18_pad_cfg();
}
