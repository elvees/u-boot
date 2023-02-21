// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/kernel.h>

#include "../common/mcom03-common.h"

DECLARE_GLOBAL_DATA_PTR;

void board_pads_cfg(void)
{
	/* U-Boot don't have pinctrl driver, so switch pad voltage manually */
	lsperiph1_v18_pad_cfg();
}

int power_init_board(void)
{
	u32 val;

	/* LTE module requires a pulse on PWRKEY input pin to turn on.
	 * Pulse duration must be at least 500ms.
	 * TODO: Move it to userspace.
	 */
	/* Reset deassert */
	val = readl(MFBSP1_DIR);
	val |= BIT(7);
	writel(val, MFBSP1_DIR);

	val = readl(MFBSP1_DR);
	val |= BIT(7);
	writel(val, MFBSP1_DR);

	/* PWR OFF */
	val = readl(LSP1_GPIO_SWPORTD_DDR);
	val |= BIT(4);
	writel(val, LSP1_GPIO_SWPORTD_DDR);

	val = readl(LSP1_GPIO_SWPORTD_DR);
	val &= ~BIT(4);
	writel(val, LSP1_GPIO_SWPORTD_DR);

	mdelay(500);

	/* PWR ON */
	val = readl(LSP1_GPIO_SWPORTD_DR);
	val |= BIT(4);
	writel(val, LSP1_GPIO_SWPORTD_DR);

	return 0;
}
