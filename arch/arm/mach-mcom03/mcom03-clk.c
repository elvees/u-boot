// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021-2023 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/iopoll.h>

#include <asm/arch/mcom03-clk.h>

#define HSP_PLL_ADDR 0x10400000
#define HSP_REFCLK_ADDR 0x1040000c
#define LSP0_PLL_ADDR 0x1680000
#define LSP1_PLL_ADDR 0x17e0000
#define MEDIA_PLL0_ADDR 0x1320000
#define MEDIA_PLL1_ADDR 0x1320010
#define MEDIA_PLL2_ADDR 0x1320020
#define LSP1_I2S_UCG_RSTN_PPOLICY 0x17e0008
#define LSP1_I2S_UCG_RSTN_PSTATUS 0x17e000c
#define SDR_PLL0_ADDR 0x1910000
#define SDR_PLL1_ADDR 0x1910008
#define SDR_PLL2_ADDR 0x1910010
#define SERV_PLL_ADDR 0x1f001000

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

#define SDR_URB_PCI0_CTL		0x1910050
#define SDR_URB_PCI1_CTL		0x1910054
#define SDR_URB_PCIE_CTL_ENABLE_CLK	BIT(0)
#define SDR_URB_PCIE_CTL_PAD_EN		BIT(4)

#define PP_ON 0x10

enum pll_id {
	HSP_PLL,
	LSP0_PLL,
	LSP1_PLL,
	MEDIA_PLL0,
	MEDIA_PLL1,
	MEDIA_PLL2,
	SDR_PLL0,
	SDR_PLL1,
	SDR_PLL2,
	SERV_PLL,
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
	{ SDR_PLL1, 27000000, 648000000, 0, 95, 3 },
	{ SDR_PLL2, 27000000, 459000000, 0, 101, 5 },
};

struct ucg_channel {
	int ucg_id;
	int chan_id;
	int div;
};

static struct ucg_channel ucg_hsp_channels[] = {
	{0, 0, 5},	/* HSPERIPH UCG0 SYS		225 MHz */
	{0, 1, 5},	/* HSPERIPH UCG0 DMA		225 MHz */
	{0, 2, 12},	/* HSPERIPH UCG0 CTR		93.75 MHz */
	{0, 3, 5},	/* HSPERIPH UCG0 SPRAM		225 MHz */
	{0, 4, 9},	/* HSPERIPH UCG0 EMAC0		125 MHz */
	{0, 5, 9},	/* HSPERIPH UCG0 EMAC1		125 MHz */
	{0, 6, 9},	/* HSPERIPH UCG0 USB0		125 MHz */
	{0, 7, 9},	/* HSPERIPH UCG0 USB1		125 MHz */
	{0, 8, 9},	/* HSPERIPH UCG0 AXI NFC	125 MHz */
	{0, 9, 5},	/* HSPERIPH UCG0 PDMA2		225 MHz */
	{0, 10, 12},	/* HSPERIPH UCG0 AXI SDMMC0	93.75 MHz */
	{0, 11, 12},	/* HSPERIPH UCG0 AXI SDMMC1	93.75 MHz */
	{0, 12, 12},	/* HSPERIPH UCG0 AXI QSPI	93.75 MHz */
	{1, 0, 9},	/* HSPERIPH UCG1 SDMMC0 XIN	125 MHz */
	{1, 1, 9},	/* HSPERIPH UCG1 SDMMC1 XIN	125 MHz */
	{1, 2, 12},	/* HSPERIPH UCG1 NFC		93.75 MHz */
	{1, 3, 45},	/* HSPERIPH UCG1 QSPI		25 MHz */
	{1, 4, 15},	/* HSPERIPH UCG1 UltraSOC	75 MHz */
	{2, 0, 45},	/* HSPERIPH UCG2 EMAC0 1588	25 MHz */
	{2, 2, 45},	/* HSPERIPH UCG2 EMAC1 1588	25 MHz */
	{2, 1, 9},	/* HSPERIPH UCG2 EMAC0 TXC	125 MHz */
	{2, 3, 9},	/* HSPERIPH UCG2 EMAC1 TXC	125 MHz */
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
	{0, 1, 6},	/* LSPERIPH1 UCG0 I2C1		102.375 MHz */
	{0, 2, 6},	/* LSPERIPH1 UCG0 I2C2		102.375 MHz */
	{0, 3, 6},	/* LSPERIPH1 UCG0 I2C3		102.375 MHz */
	{0, 4, 7},	/* LSPERIPH1 UCG0 GPIO1_DB	87.75 MHz */
	{0, 5, 4},	/* LSPERIPH1 UCG0 SSI1		153.5625 MHz */
	{0, 6, 22},	/* LSPERIPH1 UCG0 UART0		27.920454 MHz */
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
};

static struct ucg_channel ucg_sdr_channels[] = {
	{0, 0, 18},	/* SDR UCG0 CLK_CFG		105 MHz */
	{0, 1, 6},	/* SDR UCG0 EXT_CLK		315 MHz */
	{0, 2, 9},	/* SDR UCG0 INT_CLK		210 MHz */
	{0, 3, 9},	/* SDR UCG0 PCI_CLK		210 MHz */
	{0, 4, -6},	/* SDR UCG0 VCU_ACLK		off (315 MHz) */
	{0, 5, -3},	/* SDR UCG0 ACC0_CLK		off (630 MHz) */
	{0, 6, -3},	/* SDR UCG0 ACC1_CLK		off (630 MHz) */
	{0, 7, -3},	/* SDR UCG0 ACC2_CLK		off (630 MHz) */
	{0, 8, 37},	/* SDR UCG0 AUX_PCI_CLK		51 MHz */
	{0, 9, -6},	/* SDR UCG0 GNSS_CLK		off (315 MHz) */
	{0, 10, -3},	/* SDR UCG0 DFE_ALT_CLK		off (630 MHz) */
	{0, 11, -18},	/* SDR UCG0 VCU_TCK		off (105 MHz) */
	{0, 12, -18},	/* SDR UCG0 LVDS_CLK		off (105 MHz) */
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
		   unsigned long *pll_addr, enum pll_id *pll_ids, int pll_num,
		   u16 *sync_value)
{
	unsigned long chan_addr;
	int i, ret, max_ucg_id = 0;
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
		chan_addr = ucg_ctr_addr_get(ucg_channels[i].ucg_id,
					     ucg_channels[i].chan_id);

		val = readl(chan_addr);
		val &= ~UCG_CTR_DIV_COEFF;
		val |= FIELD_PREP(UCG_CTR_DIV_COEFF, abs(ucg_channels[i].div));
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
			if (ucg_channels[i].div >= 0)
				val |= UCG_CTR_CLK_EN;
			else
				val &= ~UCG_CTR_CLK_EN;

			writel(val, chan_addr);
			if (ucg_channels[i].div >= 0) {
				ret = readl_poll_timeout(chan_addr, val,
							 FIELD_GET(UCG_CTR_QFSM_STATE, val) ==
								Q_FSM_RUN,
							 1000);
				if (ret)
					return ret;
			}
		}
	}

	if (sync_value) {
		for (i = 0; i < chans_num; i++)
			if (ucg_channels[i].ucg_id > max_ucg_id)
				max_ucg_id = ucg_channels[i].ucg_id;

		for (i = 0; i <= max_ucg_id; i++)
			if (sync_value[i])
				writel(sync_value[i], ucg_bp_addr_get(i) + 0x4);
	}

	/* Disable bypass */
	for (i = 0; i < chans_num; i++) {
		val = readl(ucg_bp_addr_get(ucg_channels[i].ucg_id));
		val &= ~BIT(ucg_channels[i].chan_id);
		writel(val, ucg_bp_addr_get(ucg_channels[i].ucg_id));
	}

	return 0;
}

int clk_cfg(void)
{
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
		      HSP_REFCLK_ADDR, 0x0,
		      (unsigned long []) { HSP_PLL_ADDR },
		      (enum pll_id []) { HSP_PLL }, 1, NULL);
	if (ret)
		return ret;

	ret = ucg_cfg(ucg_lsp0_channels, ARRAY_SIZE(ucg_lsp0_channels),
		      lsp0_ucg_ctr_addr_get, lsp0_ucg_bp_addr_get,
		      0, 0x0,
		      (unsigned long []) { LSP0_PLL_ADDR },
		      (enum pll_id []) { LSP0_PLL }, 1, NULL);
	if (ret)
		return ret;

	ret = ucg_cfg(ucg_lsp1_channels, ARRAY_SIZE(ucg_lsp1_channels),
		      lsp1_ucg_ctr_addr_get, lsp1_ucg_bp_addr_get,
		      0, 0x0,
		      (unsigned long []) { LSP1_PLL_ADDR },
		      (enum pll_id []) { LSP1_PLL }, 1, NULL);
	if (ret)
		return ret;

	return ucg_cfg(ucg_media_channels, ARRAY_SIZE(ucg_media_channels),
		       media_ucg_ctr_addr_get, media_ucg_bp_addr_get,
		       0, 0x0,
		       (unsigned long []) {
			      MEDIA_PLL0_ADDR,
			      MEDIA_PLL1_ADDR,
			      MEDIA_PLL2_ADDR,
		       },
		       (enum pll_id []) {
			      MEDIA_PLL0,
			      MEDIA_PLL1,
			      MEDIA_PLL2,
		       }, 3, NULL);
}

int clk_cfg_sdr(void)
{
	int ret = ucg_cfg(ucg_sdr_channels, ARRAY_SIZE(ucg_sdr_channels),
			  sdr_ucg_ctr_addr_get, sdr_ucg_bp_addr_get,
			  0, 0x0,
			  (unsigned long []) {
				SDR_PLL0_ADDR,
				SDR_PLL1_ADDR,
				SDR_PLL2_ADDR
			  },
			  (enum pll_id []) {
				SDR_PLL0,
				SDR_PLL1,
				SDR_PLL2
			  }, 3, NULL);
	if (ret)
		return ret;
	// Enable DSP clocks
	writel(SDR_URB_DSP_CTL_ENABLE_CLK, SDR_URB_DSP_CTL);
	// Enable PCIe external clocks
	writel(SDR_URB_PCIE_CTL_ENABLE_CLK | SDR_URB_PCIE_CTL_PAD_EN,
	       SDR_URB_PCI0_CTL);
	writel(SDR_URB_PCIE_CTL_ENABLE_CLK | SDR_URB_PCIE_CTL_PAD_EN,
	       SDR_URB_PCI1_CTL);

	return 0;
}
