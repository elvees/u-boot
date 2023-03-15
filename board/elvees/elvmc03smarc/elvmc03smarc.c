// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <dm.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/kernel.h>

#include "../common/mcom03-common.h"

DECLARE_GLOBAL_DATA_PTR;

int power_init_board(void)
{
	u32 val;

	if (of_machine_is_compatible("elvees,elvmc03smarc-r1.0")) {
		/* Setup CARRIER_PWR_ON signal on ELV-MC03-SMARC */
		/* TODO: rework to use GPIO driver */
		val = readl(LSP1_GPIO_SWPORTC_DDR);
		val |= BIT(5);
		writel(val, LSP1_GPIO_SWPORTC_DDR);

		val = readl(LSP1_GPIO_SWPORTC_DR);
		val |= BIT(5);
		writel(val, LSP1_GPIO_SWPORTC_DR);

		/* Let carrier standby circuits switch on, 1ms should be enough */
		mdelay(1);

		/* Setup CARRIER_STBY# signal on ELV-MC03-SMARC */
		val = readl(LSP1_GPIO_SWPORTD_DDR);
		val |= BIT(0);
		writel(val, LSP1_GPIO_SWPORTD_DDR);

		val = readl(LSP1_GPIO_SWPORTD_DR);
		val |= BIT(0);
		writel(val, LSP1_GPIO_SWPORTD_DR);

		/* Delay >100ms from CARRIER_PWR_ON to RESET_OUT# signals.
			* The SMARC specification does not explain why this 100ms is needed.
			* Perhaps we don't need it at all.
			*/
		mdelay(100);

		if (of_machine_is_compatible("radxa,rockpi-n10"))
			return 0;

		/* Setup RESET_OUT# signal on ELV-MC03-SMARC */
		val = readl(LSP1_GPIO_SWPORTD_DDR);
		val |= BIT(7);
		writel(val, LSP1_GPIO_SWPORTD_DDR);

		val = readl(LSP1_GPIO_SWPORTD_DR);
		val |= BIT(7);
		writel(val, LSP1_GPIO_SWPORTD_DR);
	};

	if (of_machine_is_compatible("elvees,elvmc03smarc-r2.2")) {
		/* Setup CARRIER_STBY# signal on ELV-MC03-SMARC r2.2*/
		val = readl(LSP1_GPIO_SWPORTC_DDR);
		val |= BIT(5);
		writel(val, LSP1_GPIO_SWPORTC_DDR);

		val = readl(LSP1_GPIO_SWPORTC_DR);
		val |= BIT(5);
		writel(val, LSP1_GPIO_SWPORTC_DR);
	}

	return 0;
}

void board_pads_cfg(void)
{
	/* U-Boot don't have pinctrl driver, so switch pad voltage manually */
	lsperiph1_v18_pad_cfg();
}
