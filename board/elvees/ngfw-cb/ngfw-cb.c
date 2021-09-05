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

int power_init_board(void)
{
	u32 val;

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

	/* Setup RESET_OUT# signal on ELV-MC03-SMARC */
	val = readl(LSP1_GPIO_SWPORTD_DDR);
	val |= BIT(7);
	writel(val, LSP1_GPIO_SWPORTD_DDR);

	val = readl(LSP1_GPIO_SWPORTD_DR);
	val |= BIT(7);
	writel(val, LSP1_GPIO_SWPORTD_DR);

	return 0;
}

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
