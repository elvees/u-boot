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
#define LSP0_PLL_ADDR 0x1680000
#define LSP1_PLL_ADDR 0x17e0000
#define MEDIA_PLL0_ADDR 0x1320000
#define MEDIA_PLL1_ADDR 0x1320010
#define MEDIA_PLL2_ADDR 0x1320020
#define MEDIA_PLL3_ADDR 0x1320030
#define LSP1_I2S_UCG_RSTN_PPOLICY 0x17e0008
#define LSP1_I2S_UCG_RSTN_PSTATUS 0x17e000c
#define SDR_PLL0_ADDR 0x1910000
#define SDR_PLL1_ADDR 0x1910008
#define SDR_PLL2_ADDR 0x1910010

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

#define SDR_URB_DSP_CTL 0x191004C
#define SDR_URB_DSP_CTL_ENABLE_CLK (BIT(8) | BIT(9))

#define PP_ON 0x10

enum pll_id {
	HSP_PLL,
	LSP0_PLL,
	LSP1_PLL,
	MEDIA_PLL0,
	MEDIA_PLL1,
	MEDIA_PLL2,
	MEDIA_PLL3,
	SDR_PLL0,
	SDR_PLL1,
	SDR_PLL2,
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
	{ LSP0_PLL, 27000000, 189000000, 0, 111, 15 },
	{ LSP1_PLL, 27000000, 614250000, 0, 90, 3 },
	{ MEDIA_PLL0, 27000000, 1998000000, 0, 73, 0 },
	{ MEDIA_PLL1, 27000000, 594000000, 0, 131, 5 },
	{ MEDIA_PLL2, 27000000, 495000000, 0, 109, 5 },

	{ SDR_PLL0, 27000000, 1890000000, 0, 69, 0 },
	{ SDR_PLL1, 27000000, 657000000, 0, 72, 2 },
	{ SDR_PLL2, 27000000, 459000000, 0, 16, 0 },
	/* UCG3 channel 5 used as workaround for DISP_PIXCLK (UCG1 channel 2).
	 * Setup PLL3 to frequency that can be divided by DISP_PIXCLK.
	 */
	{ MEDIA_PLL3, 27000000, 594000000, 0, 131, 5 },
};

struct ucg_channel {
	int ucg_id;
	int chan_id;
	int div;
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

static struct ucg_channel ucg_lsp0_channels[] = {
	{0, 0, 2},	/* LSPERIPH0 UCG0 SYS		94.5 MHz */
	{0, 1, 14},	/* LSPERIPH0 UCG0 UART3		13.5 MHz */
	{0, 2, 14},	/* LSPERIPH0 UCG0 UART1		13.5 MHz */
	{0, 3, 14},	/* LSPERIPH0 UCG0 UART2		13.5 MHz */
	{0, 4, 2},	/* LSPERIPH0 UCG0 SPI0		94.5 MHz */
	{0, 5, 2},	/* LSPERIPH0 UCG0 I2C0		94.5 MHz */
	{0, 6, 189},	/* LSPERIPH0 UCG0 DBCLK		1 MHz */
};

static struct ucg_channel ucg_lsp1_channels[] = {
	{0, 0, 7},	/* LSPERIPH1 UCG0 SYS		87.75 MHz */
	{0, 1, 6},	/* LSPERIPH1 UCG0 I2C0		102.375 MHz */
	{0, 2, 6},	/* LSPERIPH1 UCG0 I2C1		102.375 MHz */
	{0, 3, 6},	/* LSPERIPH1 UCG0 I2C2		102.375 MHz */
	{0, 4, 7},	/* LSPERIPH1 UCG0 GPIO1_DB	87.75 MHz */
	{0, 5, 4},	/* LSPERIPH1 UCG0 SSI1		153.5625 MHz */
	{0, 6, -1},	/* LSPERIPH1 UCG0 UART0		27 MHz (bypass) */
	{0, 7, 7},	/* LSPERIPH1 UCG0 TIMERS	87.75 MHz */
	{0, 8, 7},	/* LSPERIPH1 UCG0 PWM0		87.75 MHz */
	{0, 9, 7},	/* LSPERIPH1 UCG0 WDT1		87.75 MHz */
	{1, 0, 50},	/* LSPERIPH1 UCG1 I2S		12.285 MHz */
};

static struct ucg_channel ucg_media_channels[] = {
	{0, 0, 16},	/* MEDIA UCG0 SYS		124.875 MHz */
	{0, 1, 10},	/* MEDIA UCG0 ISP		199.8 MHz */
	{1, 0, 2},	/* MEDIA UCG1 DISP_ACLK		297 MHz */
	{1, 1, 4},	/* MEDIA UCG1 DISP_MCLK		148.5 MHz */
	{1, 2, 4},	/* MEDIA UCG1 DISP_PIXCLK	148.5 MHz */
	{2, 0, 2},	/* MEDIA UCG2 GPU_SYS		247.5 MHz */
	{2, 1, 2},	/* MEDIA UCG2 GPU_MEM		247.5 MHz */
	{2, 2, 2},	/* MEDIA UCG2 GPU_CORE		247.5 MHz */
	{3, 0, -1},	/* MEDIA UCG3 MIPI_RX_REF	27 MHz (bypass) */
	{3, 1, -1},	/* MEDIA UCG3 MIPI_RX0_CFG	27 MHz (bypass) */
	{3, 2, -1},	/* MEDIA UCG3 MIPI_RX1_CFG	27 MHz (bypass) */
	{3, 3, -1},	/* MEDIA UCG3 MIPI_TX_REF	27 MHz (bypass) */
	{3, 4, -1},	/* MEDIA UCG3 MIPI_TX_CFG	27 MHz (bypass) */
	{3, 5, 4},	/* MEDIA UCG3 CMOS0		148.5 as DISP_PIXCLK */
	{3, 6, -1},	/* MEDIA UCG3 CMOS1		27 MHz (bypass) */
	{3, 7, 30},	/* MEDIA UCG3 MIPI_TXCLKESC	19.8 MHz */
	{3, 8, 2},	/* MEDIA UCG3 VPU_CLK		297 MHz */
};

static struct ucg_channel ucg_sdr_channels[] = {
	{0, 0, 9},	/* SDR UCG0 CLK_CFG		210 MHz */
	{0, 1, 8},	/* SDR UCG0 EXT_CLK		236 MHz */
	{0, 2, 4},	/* SDR UCG0 INT_CLK		472 MHz */
	{0, 3, 9},	/* SDR UCG0 PCI_CLK		210 MHz */
	{0, 4, 3},	/* SDR UCG0 VCU_CLK		630 MHz */
	{0, 5, 3},	/* SDR UCG0 ACC0_CLK		630 MHz */
	{0, 6, 3},	/* SDR UCG0 ACC1_CLK		630 MHz */
	{0, 7, 3},	/* SDR UCG0 ACC2_CLK		630 MHz */
	{0, 8, 37},	/* SDR UCG0 AUX_PCI_CLK		51 MHz */
	{0, 9, 3},	/* SDR UCG0 GNSS_CLK		630 MHz */
	{0, 10, 3},	/* SDR UCG0 DFE_ALT_CLK		630 MHz */
	{0, 11, 9},	/* SDR UCG0 VCU_CLK		210 MHz */
	{0, 12, 9},	/* SDR UCG0 LVDS_CLK		210 MHz */
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

unsigned long lsp0_ucg_ctr_addr_get(int ucg_id, int chan_id)
{
	return 0x1690000 + ucg_id * 0x10000 + chan_id * 0x4;
}

unsigned long lsp0_ucg_bp_addr_get(int ucg_id)
{
	return 0x1690040 + ucg_id * 0x10000;
}

unsigned long lsp1_ucg_ctr_addr_get(int ucg_id, int chan_id)
{
	return 0x17c0000 + ucg_id * 0x10000 + chan_id * 0x4;
}

unsigned long lsp1_ucg_bp_addr_get(int ucg_id)
{
	return 0x17c0040 + ucg_id * 0x10000;
}

unsigned long media_ucg_ctr_addr_get(int ucg_id, int chan_id)
{
	return 0x1320040 + ucg_id * 0x80 + chan_id * 0x4;
}

unsigned long media_ucg_bp_addr_get(int ucg_id)
{
	return 0x1320080 + ucg_id * 0x80;
}

unsigned long sdr_ucg_ctr_addr_get(int ucg_id, int chan_id)
{
	return 0x1900000 + chan_id * 0x4;
}

unsigned long sdr_ucg_bp_addr_get(int ucg_id)
{
	return 0x1900040;
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
		   unsigned long *pll_addr, enum pll_id *pll_ids, int pll_num)
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
	if (refclk_addr)
		writel(refclk_mask, refclk_addr);

	for (i = 0; i < pll_num; i++) {
		ret = pll_cfg(pll_ids[i], pll_addr[i]);
		if (ret)
			return ret;
	}

	/* Set dividers */
	for (i = 0; i < chans_num; i++) {
		if (ucg_channels[i].div == -1)
			continue;

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

		if (ucg_channels[i].div == -1)
			val |= BIT(ucg_channels[i].chan_id);
		else
			val &= ~BIT(ucg_channels[i].chan_id);

		writel(val, ucg_bp_addr_get(ucg_channels[i].ucg_id));
	}

	return 0;
}

int clk_cfg(void)
{
	enum pll_id pll_hsp = HSP_PLL;
	enum pll_id pll_lsp0 = LSP0_PLL;
	enum pll_id pll_lsp1 = LSP1_PLL;
	enum pll_id pll_media[] = { MEDIA_PLL0, MEDIA_PLL1,
				    MEDIA_PLL2, MEDIA_PLL3 };
	enum pll_id pll_sdr[] = { SDR_PLL0, SDR_PLL1, SDR_PLL2 };
	unsigned long hsp_pll_addr[] = { HSP_PLL_ADDR };
	unsigned long lsp0_pll_addr[] = { LSP0_PLL_ADDR };
	unsigned long lsp1_pll_addr[] = { LSP1_PLL_ADDR };
	unsigned long media_pll_addr[] = { MEDIA_PLL0_ADDR, MEDIA_PLL1_ADDR,
					   MEDIA_PLL2_ADDR, MEDIA_PLL3_ADDR };
	unsigned long sdr_pll_addr[] = { SDR_PLL0_ADDR, SDR_PLL1_ADDR, SDR_PLL2_ADDR };
	u32 val;
	int ret;

	/* I2S RSTN muset be enabled before LSP1 UCGs setup */
	writel(PP_ON, LSP1_I2S_UCG_RSTN_PPOLICY);
	ret = readl_poll_timeout(LSP1_I2S_UCG_RSTN_PSTATUS, val, val == PP_ON,
				 1000);
	if (ret)
		return ret;

	ret = ucg_cfg(ucg_hsp_channels, ARRAY_SIZE(ucg_hsp_channels),
		      hsp_ucg_ctr_addr_get, hsp_ucg_bp_addr_get,
		      HSP_REFCLK_ADDR, 0x0, hsp_pll_addr, &pll_hsp,
		      ARRAY_SIZE(hsp_pll_addr));
	if (ret)
		return ret;

	ret = ucg_cfg(ucg_lsp0_channels, ARRAY_SIZE(ucg_lsp0_channels),
		      lsp0_ucg_ctr_addr_get, lsp0_ucg_bp_addr_get,
		      0, 0x0, lsp0_pll_addr, &pll_lsp0,
		      ARRAY_SIZE(lsp0_pll_addr));
	if (ret)
		return ret;

	ret = ucg_cfg(ucg_lsp1_channels, ARRAY_SIZE(ucg_lsp1_channels),
		      lsp1_ucg_ctr_addr_get, lsp1_ucg_bp_addr_get,
		      0, 0x0, lsp1_pll_addr, &pll_lsp1,
		      ARRAY_SIZE(lsp1_pll_addr));
	if (ret)
		return ret;

	ret = ucg_cfg(ucg_media_channels, ARRAY_SIZE(ucg_media_channels),
		      media_ucg_ctr_addr_get, media_ucg_bp_addr_get,
		      0, 0x0, media_pll_addr, pll_media,
		      ARRAY_SIZE(media_pll_addr));
	if (ret)
		return ret;

	ret = ucg_cfg(ucg_sdr_channels, ARRAY_SIZE(ucg_sdr_channels),
		      sdr_ucg_ctr_addr_get, sdr_ucg_bp_addr_get,
		      0, 0x0, sdr_pll_addr, pll_sdr,
		      ARRAY_SIZE(sdr_pll_addr));
	if (ret)
		return ret;
	// Enable DSP clocks
	writel(SDR_URB_DSP_CTL_ENABLE_CLK, SDR_URB_DSP_CTL);

	return 0;
}