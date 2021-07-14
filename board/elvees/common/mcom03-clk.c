// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/iopoll.h>

#include "mcom03-clk.h"

#define HSP_PLL_ADDR 0x10400000
#define HSP_REFCLK_ADDR 0x1040000c

#define PLL_CFG_SEL GENMASK(7, 0)
#define PLL_CFG_MAN BIT(9)
#define PLL_CFG_OD GENMASK(13, 10)
#define PLL_CFG_NF GENMASK(26, 14)
#define PLL_CFG_NR GENMASK(30, 27)
#define PLL_CFG_LOCK BIT(31)

#define UCG_CTR_LPI_EN BIT(0)
#define UCG_CTR_CLK_EN BIT(1)
#define UCG_CTR_CLK_EN_STS GENMASK(4, 2)
#define UCG_CTR_QACTIVE_CTL_EN BIT(6)
#define UCG_CTR_QFSM_STATE GENMASK(9, 7)
#define UCG_CTR_DIV_COEFF GENMASK(29, 10)
#define UCG_CTR_DIV_LOCK BIT(30)

enum pll_id {
	HSP_PLL,
	LSP0_PLL
};

struct pll_settings {
	enum pll_id id;
	u32 ifreq;
	u32 ofreq;
	u8 nr;
	u16 nf;
	u8 od;
};

static struct pll_settings pll_settings[] = {
	{ HSP_PLL, 27000000, 1125000000, 2, 249, 1 },
};

struct ucg_channel {
	int ucg_id;
	int chan_id;
	u32 div;
};

static struct ucg_channel ucg_hsp_channels[] = {
	{0, 0, 4},	/* HSPERIPH UCG0 SYS		375 MHz */
	{0, 1, 4},	/* HSPERIPH UCG0 DMA		375 MHz */
	{0, 2, 12},	/* HSPERIPH UCG0 CTR		93.75 MHz */
	{0, 3, 4},	/* HSPERIPH UCG0 SPRAM		375 MHz */
	{0, 4, 9},	/* HSPERIPH UCG0 EMAC0		125 MHz */
	{0, 5, 9},	/* HSPERIPH UCG0 EMAC1		125 MHz */
	{0, 6, 9},	/* HSPERIPH UCG0 USB0		125 MHz */
	{0, 7, 9},	/* HSPERIPH UCG0 USB1		125 MHz */
	{0, 8, 9},	/* HSPERIPH UCG0 AXI NFC	125 MHz */
	{0, 9, 4},	/* HSPERIPH UCG0 PDMA2		375 MHz */
	{0, 10, 12},	/* HSPERIPH UCG0 AXI SDMMC0	93.75 MHz */
	{0, 11, 12},	/* HSPERIPH UCG0 AXI SDMMC1	93.75 MHz */
	{0, 12, 12},	/* HSPERIPH UCG0 AXI QSPI	93.75 MHz */
	{1, 0, 9},	/* HSPERIPH UCG1 SDMMC0 XIN	125 MHz */
	{1, 1, 9},	/* HSPERIPH UCG1 SDMMC1 XIN	125 MHz */
	{1, 2, 12},	/* HSPERIPH UCG1 NFC		93.75 MHz */
	{1, 3, 45},	/* HSPERIPH UCG1 QSPI		25 MHz */
	{1, 4, 9},	/* HSPERIPH UCG1 UltraSOC	125 MHz */
	{2, 0, 45},	/* HSPERIPH UCG2 EMAC0 1588	25 MHz */
	{2, 1, 45},	/* HSPERIPH UCG2 EMAC0 TXC	25 MHz */
	{2, 2, 45},	/* HSPERIPH UCG2 EMAC1 1588	25 MHz */
	{2, 3, 45},	/* HSPERIPH UCG2 EMAC1 TXC	25 MHz */
	{3, 0, 45},	/* HSPERIPH UCG3 USB0 ref	25 MHz */
	{3, 1, 45},	/* HSPERIPH UCG3 USB0 suspend	25 MHz */
	{3, 2, 45},	/* HSPERIPH UCG3 USB1 ref	25 MHz */
	{3, 3, 45},	/* HSPERIPH UCG3 USB1 suspend	25 MHz */
};

enum ucg_qfsm_state {
	Q_FSM_STOPPED = 0,
	Q_FSM_CLK_EN = 1,
	Q_FSM_REQUEST = 2,
	Q_FSM_DENIED = 3,
	Q_FSM_EXIT = 4,
	Q_FSM_RUN = 6,
	Q_FSM_CONTINUE = 7
};

unsigned long hsp_ucg_ctr_addr_get(int ucg_id, int chan_id)
{
	return 0x10410000 + ucg_id * 0x10000 + chan_id * 0x4;
}

unsigned long hsp_ucg_bp_addr_get(int ucg_id)
{
	return 0x10410040 + ucg_id * 0x10000;
}

static int pll_settings_get(int pll_id, struct pll_settings *settings)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pll_settings); i++) {
		if (pll_id == pll_settings[i].id &&
		    pll_settings[i].ifreq == CONFIG_MCOM03_XTAL_FREQ)  {
			settings->id = pll_settings[i].id;
			settings->ifreq = pll_settings[i].ifreq;
			settings->ofreq = pll_settings[i].ofreq;
			settings->nr = pll_settings[i].nr;
			settings->nf = pll_settings[i].nf;
			settings->od = pll_settings[i].od;
			break;
		}
	}

	if (i == ARRAY_SIZE(pll_settings))
		return -EINVAL;

	return 0;
}

static int pll_cfg(int pll_id, unsigned long pll_addr)
{
	struct pll_settings settings;
	u32 val;
	int ret;

	ret = pll_settings_get(pll_id, &settings);
	if (ret)
		return ret;

	val = FIELD_PREP(PLL_CFG_SEL, 1) |
	      FIELD_PREP(PLL_CFG_MAN, 1) |
	      FIELD_PREP(PLL_CFG_NR, settings.nr) |
	      FIELD_PREP(PLL_CFG_NF, settings.nf) |
	      FIELD_PREP(PLL_CFG_OD, settings.od);
	writel(val, pll_addr);

	ret = readl_poll_timeout(pll_addr, val, val & PLL_CFG_LOCK, 1000);
	if (ret)
		return ret;

	return 0;
}

static int ucg_cfg(struct ucg_channel *ucg_channels, int chans_num,
		   unsigned long (*ucg_ctr_addr_get)(int, int),
		   unsigned long (*ucg_bp_addr_get)(int),
		   unsigned long refclk_addr, u32 refclk_mask,
		   unsigned long pll_addr, enum pll_id pll_id)
{
	unsigned long chan_addr;
	int i, ret;
	u32 val;

	/* Enable bypass on all enabled channels */
	for (i = 0; i < chans_num; i++) {
		chan_addr = ucg_ctr_addr_get(ucg_channels[i].ucg_id,
					     ucg_channels[i].chan_id);

		val = readl(chan_addr);
		if (FIELD_GET(UCG_CTR_QFSM_STATE, val) == Q_FSM_RUN) {
			val = readl(ucg_bp_addr_get(ucg_channels[i].ucg_id));
			val |= BIT(ucg_channels[i].chan_id);
			writel(val, ucg_bp_addr_get(ucg_channels[i].ucg_id));
		}
	}

	/* Set reference clocks for all UCGs */
	writel(refclk_mask, refclk_addr);

	ret = pll_cfg(pll_id, pll_addr);
	if (ret)
		return ret;

	/* Set dividers */
	for (i = 0; i < chans_num; i++) {
		chan_addr = ucg_ctr_addr_get(ucg_channels[i].ucg_id,
					     ucg_channels[i].chan_id);

		val = readl(chan_addr);
		val &= ~UCG_CTR_DIV_COEFF;
		val |= FIELD_PREP(UCG_CTR_DIV_COEFF, ucg_channels[i].div);
		writel(val, chan_addr);

		ret = readl_poll_timeout(chan_addr, val,
					 val & UCG_CTR_DIV_LOCK, 1000);
		if (ret)
			return ret;
	}

	/* Enable all clocks that are not in bypass mode */
	for (i = 0; i < chans_num; i++) {
		chan_addr = ucg_ctr_addr_get(ucg_channels[i].ucg_id,
					     ucg_channels[i].chan_id);

		val = readl(chan_addr);
		if (FIELD_GET(UCG_CTR_QFSM_STATE, val) != Q_FSM_RUN) {
			val &= ~UCG_CTR_LPI_EN;
			val |= UCG_CTR_CLK_EN;
			writel(val, chan_addr);

			ret = readl_poll_timeout(chan_addr, val,
						 FIELD_GET(UCG_CTR_QFSM_STATE, val) == Q_FSM_RUN,
						 1000);
			if (ret)
				return ret;
		}
	}

	/* Disable bypass */
	for (i = 0; i < chans_num; i++) {
		val = readl(ucg_bp_addr_get(ucg_channels[i].ucg_id));

		if (val & BIT(ucg_channels[i].chan_id))
			val &= ~BIT(ucg_channels[i].chan_id);

		writel(val, ucg_bp_addr_get(ucg_channels[i].ucg_id));
	}

	return 0;
}

int clk_cfg(void)
{
	int ret;

	ret = ucg_cfg(&ucg_hsp_channels[0], ARRAY_SIZE(ucg_hsp_channels),
		      hsp_ucg_ctr_addr_get, hsp_ucg_bp_addr_get,
		      HSP_REFCLK_ADDR, 0x0, HSP_PLL_ADDR, HSP_PLL);
	if (ret)
		return ret;

	return 0;
}
