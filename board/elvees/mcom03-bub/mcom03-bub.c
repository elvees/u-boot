// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/kernel.h>

#include "../common/mcom03-common.h"

DECLARE_GLOBAL_DATA_PTR;

void board_pads_cfg(void)
{
	int i;

	nand_pad_cfg();


	/* Set EMAC pads drive strength to 12 mA for data and 8 mA for clock.
	 * Required for correct operation at 125 MHz 3.3V. See #MCOM03SW-823
	 */
	pad_set_ctl(HSP_URB_EMAC0_TX_PADCFG, 0x3f);
	pad_set_ctl(HSP_URB_EMAC0_TXC_PADCFG, 0xf);
	pad_set_ctl(HSP_URB_EMAC1_TX_PADCFG, 0x3f);
	pad_set_ctl(HSP_URB_EMAC1_TXC_PADCFG, 0xf);

	for (i = 0; i < 4; i++)
		i2c_pad_cfg(i);
}
