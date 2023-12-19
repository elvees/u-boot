// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 RnD Center "ELVEES", JSC
 */

#define LOG_CATEGORY UCLASS_CLK

#include <asm/io.h>
#include <clk-uclass.h>
#include <common.h>
#include <div64.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <dt-bindings/clock/mcom03-clock.h>
#include <linux/bitfield.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <mapmem.h>
#include <regmap.h>
#include <syscon.h>

DECLARE_GLOBAL_DATA_PTR;

#define PLL_VCO_MAX_FREQ 3600000000UL
#define PLL_VCO_MIN_FREQ 720000000UL
#define PLL_NF_MAX 4096
#define UCG_MAX_DIVIDER 0xfffffU
#define BP_CTR_REG 0x40
#define UCG_SIZE 0x48
#define PLL_SIZE 0x8
#define MAX_UCG_PER_PLL 4
#define SUBSYSTEM_CPU 0
#define SUBSYSTEM_SDR 1
#define SUBSYSTEM_MEDIA 2
#define SUBSYSTEM_CORE 3
#define SUBSYSTEM_HSP 4
#define SUBSYSTEM_LSP0 5
#define SUBSYSTEM_LSP1 6
#define SUBSYSTEM_DDR 7
#define SUBSYSTEM_TOP 8
#define SUBSYSTEM_LSP1_I2S_UCG 9
#define LSP1_I2S_UCG_RSTN_PSTATUS 0x17e000c
#define PP_ON 0x10

#define LPI_EN BIT(0)
#define CLK_EN BIT(1)
#define Q_FSM_STATE GENMASK(9, 7)
#define DIV_COEFF GENMASK(29, 10)
#define DIV_LOCK BIT(30)

#define Q_STOPPED 0
#define Q_RUN FIELD_PREP(Q_FSM_STATE, 0x6)

#define PLL_SEL GENMASK(7, 0)
#define PLL_MAN BIT(9)
#define PLL_OD GENMASK(13, 10)
#define PLL_NF GENMASK(26, 14)
#define PLL_NR GENMASK(30, 27)
#define PLL_LOCK BIT(31)

enum mcom03_clk_type {
	UNKNOWN,
	PLL,
	UCG,
	REFMUX,
};

enum property_type {
	MAX_NR,
	UCG_FIXED_FREQ_MASK,
	UCG_ROUND_UP_MASK,
};

struct mcom03_clk_plat {
	struct clk xti_clk;
	struct regmap *serv_urb;
	void __iomem *i2s_ucg_rstn_pstatus;
};

struct mcom03_pll {
	const char *name;
	struct clk clk;
	void __iomem *iobase;
	fdt_addr_t base_addr;
	u8 ucg_ids[MAX_UCG_PER_PLL];
	u8 ucg_id_len;
	u8 max_nr;
	u8 subsystem;
};

struct mcom03_ucg {
	struct clk clks[16];
	char *names[16];
	char *parent_name;
	void __iomem *iobase;
	fdt_addr_t base_addr;
	u32 ucg_id;
	u32 first_chan_id;
	u16 fixed_freq_mask;
	u16 round_up_mask;
	u8 subsystem;
};

struct mcom03_refmux {
	const char *name;
	struct clk *clk;
	const char *parent_names[4];
	fdt_addr_t base_addr;
	u32 clk_id;
	u8 num_parents;
	u8 shift;
	u8 width;
	u8 ucg_id;
	u8 subsystem;
};

struct mcom03_pll pll_clocks[] = {
	{
		.name = "cpu_pll",
		.clk.id = CLK_CPU_PLL0,
		.max_nr = 1,
		.base_addr = 0x1000050,
		.ucg_ids = { CLK_CPU_UCG0 },
		.ucg_id_len = 1,
		.subsystem = SUBSYSTEM_CPU,
	},
	{
		.name = "service_pll",
		.clk.id = CLK_SERVICE_PLL0,
		.max_nr = 1,
		.base_addr = 0x1f001000,
		.ucg_ids = { CLK_SERVICE_UCG0 },
		.ucg_id_len = 1,
		.subsystem = SUBSYSTEM_CORE,
	},
	{
		.name = "sdr_pll0",
		.clk.id = CLK_SDR_PLL0,
		.max_nr = 1,
		.base_addr = 0x1910000,
		/* Actually several UCGs are connected to SDR_PLL0 but only
		 * SDR_UCG0 has channels that enabled by default.
		 */
		.ucg_ids = { CLK_SDR_UCG0 },
		.ucg_id_len = 1,
		.subsystem = SUBSYSTEM_SDR,
	},
	{
		.name = "sdr_pll1",
		.clk.id = CLK_SDR_PLL1,
		.max_nr = 1,
		.base_addr = 0x1910008,
		/* Only UCG_DFE_N is connected to SDR_PLL1 but its channel is
		 * disabled by default.
		 */
		.ucg_id_len = 0,
		.subsystem = SUBSYSTEM_SDR,
	},
	{
		.name = "sdr_pll2",
		.clk.id = CLK_SDR_PLL2,
		.max_nr = 1,
		.base_addr = 0x1910010,
		.ucg_id_len = 0,
		.subsystem = SUBSYSTEM_SDR,
	},
	{
		.name = "media_pll0",
		.clk.id = CLK_MEDIA_PLL0,
		.max_nr = 1,
		.base_addr = 0x1320000,
		.ucg_ids = { CLK_MEDIA_UCG0 },
		.ucg_id_len = 1,
		.subsystem = SUBSYSTEM_MEDIA,
	},
	{
		.name = "media_pll1",
		.clk.id = CLK_MEDIA_PLL1,
		.max_nr = 1,
		.base_addr = 0x1320010,
		.ucg_ids = { CLK_MEDIA_UCG1 },
		.ucg_id_len = 1,
		.subsystem = SUBSYSTEM_MEDIA,
	},
	{
		.name = "media_pll2",
		.clk.id = CLK_MEDIA_PLL2,
		.max_nr = 1,
		.base_addr = 0x1320020,
		.ucg_ids = { CLK_MEDIA_UCG2 },
		.ucg_id_len = 1,
		.subsystem = SUBSYSTEM_MEDIA,
	},
	{
		.name = "media_pll3",
		.clk.id = CLK_MEDIA_PLL3,
		.max_nr = 1,
		.base_addr = 0x1320030,
		.ucg_ids = { CLK_MEDIA_UCG3 },
		.ucg_id_len = 1,
		.subsystem = SUBSYSTEM_MEDIA,
	},
	{
		.name = "hsperiph_pll",
		.clk.id = CLK_HSP_PLL0,
		.max_nr = 1,
		.base_addr = 0x10400000,
		.ucg_ids = {
			CLK_HSP_UCG0,
			CLK_HSP_UCG1,
			CLK_HSP_UCG2,
			CLK_HSP_UCG3,
		},
		.ucg_id_len = 4,
		.subsystem = SUBSYSTEM_HSP,
	},
	{
		.name = "lsperiph0_pll",
		.clk.id = CLK_LSP0_PLL0,
		.max_nr = 1,
		.base_addr = 0x1680000,
		.ucg_ids = {CLK_LSP0_UCG0},
		.ucg_id_len = 1,
		.subsystem = SUBSYSTEM_LSP0,
	},
	{
		.name = "lsperiph1_pll",
		.clk.id = CLK_LSP1_PLL0,
		.max_nr = 1,
		.base_addr = 0x17e0000,
		.ucg_ids = { CLK_LSP1_UCG0, CLK_LSP1_UCG_I2S },
		.ucg_id_len = 2,
		.subsystem = SUBSYSTEM_LSP1,
	},
};

struct mcom03_refmux refmux_clocks[] = {
	{
		.name = "hsp_refmux0",
		/* Second name (CLK125) will be filled from DTB */
		.parent_names = { "hsperiph_pll", NULL },
		.num_parents = 2,
		.base_addr = 0x1040000c,
		.shift = 0,
		.width = 2,
		.clk_id = CLK_HSP_REFMUX0,
		.ucg_id = CLK_HSP_UCG0,
		.subsystem = SUBSYSTEM_HSP,
	},
	{
		.name = "hsp_refmux1",
		.parent_names = { "hsperiph_pll", NULL },
		.num_parents = 2,
		.base_addr = 0x1040000c,
		.shift = 2,
		.width = 2,
		.clk_id = CLK_HSP_REFMUX1,
		.ucg_id = CLK_HSP_UCG1,
		.subsystem = SUBSYSTEM_HSP,
	},
	{
		.name = "hsp_refmux2",
		.parent_names = { "hsperiph_pll", NULL },
		.num_parents = 2,
		.base_addr = 0x1040000c,
		.shift = 4,
		.width = 2,
		.clk_id = CLK_HSP_REFMUX2,
		.ucg_id = CLK_HSP_UCG2,
		.subsystem = SUBSYSTEM_HSP,
	},
	{
		.name = "hsp_refmux3",
		.parent_names = { "hsperiph_pll", NULL },
		.num_parents = 2,
		.base_addr = 0x1040000c,
		.shift = 6,
		.width = 2,
		.clk_id = CLK_HSP_REFMUX3,
		.ucg_id = CLK_HSP_UCG3,
		.subsystem = SUBSYSTEM_HSP,
	},
	{
		.name = "lsperiph1_refmux_i2s",
		/* Second name (CLK_I2S0_SCLK_IN) will be filled from DTB */
		.parent_names = { "lsperiph1_pll", NULL },
		.num_parents = 2,
		.base_addr = 0x17e0010,
		.shift = 0,
		.width = 1,
		.clk_id = CLK_LSP1_REFMUX_I2S,
		.ucg_id = CLK_LSP1_UCG_I2S,
		.subsystem = SUBSYSTEM_LSP1,
	},
};

static struct mcom03_ucg ucg_clocks[] = {
	/* For JESD UCG first_chan_id should be specified as
	 * (CLK_SDR_JESD*_*_SAMPLE & ~0xf) because real first channel in
	 * JESD UCGs is 1, but first_chan_id field always must contains
	 * channel 0. Also names[0] must be NULL. Example:
	 * .names = { NULL, "jesd0_rx_simple", "jesd0_rx_character", },
	 *
	 * Now some UCGs in SDR subsystem is not specified because they are
	 * not needed in U-Boot.
	 */
	{
		.ucg_id = CLK_CPU_UCG0,
		.first_chan_id = CLK_CPU_UCG0_SYS,
		.base_addr = 0x1080000,
		.names = { "cpu_sys_clk", "cpu_core_clk", "cpu_dbus_clk" },
		.parent_name = "cpu_pll",
		.subsystem = SUBSYSTEM_CPU,
	},
	{
		.ucg_id = CLK_SERVICE_UCG0,
		.first_chan_id = CLK_SERVICE_UCG0_APB,
		.base_addr = 0x1f020000,
		.names = { "service_apb_clk", "service_core_clk", "qspi0_clk",
			   "bpam_clk", "risc0_clk", "mfbsp0_clk", "mfbsp1_clk",
			   "mailbox0_clk", "pvtctr_clk", "i2c4_clk", "trng_clk",
			   "spiotp_clk", "i2c4_ext_clk", "qspi0_ext_clk",
			   "clkout_clk", "risc0_tck_clk" },
		.parent_name = "service_pll",
		.subsystem = SUBSYSTEM_CORE,
	},
	{
		.ucg_id = CLK_SDR_UCG0,
		.first_chan_id = CLK_SDR_UCG0_CFG,
		.base_addr = 0x1900000,
		.names = { "sdr_clk_cfg", "sdr_ext_aclk", "bbd_aclk",
			   "pci_aclk", "vcu_aclk", "acc0_clk", "acc1_clk",
			   "acc2_clk", "aux_pci_clk", "gnss_clk", "dfe_a_clk",
			   "vcu_tck", "lvds_clk" },
		.parent_name = "sdr_pll0",
		.subsystem = SUBSYSTEM_SDR,
	},
	{
		.ucg_id = CLK_MEDIA_UCG0,
		.first_chan_id = CLK_MEDIA_UCG0_SYSA,
		.base_addr = 0x1320040,
		.names = { "media_sys_aclk", "isp_sys_clk" },
		.parent_name = "media_pll0",
		.subsystem = SUBSYSTEM_MEDIA,
	},
	{
		.ucg_id = CLK_MEDIA_UCG1,
		.first_chan_id = CLK_MEDIA_UCG1_DISP_A,
		.base_addr = 0x13200c0,
		.names = { "disp_aclk", "disp_mclk", "disp_pxlclk" },
		.parent_name = "media_pll1",
		.subsystem = SUBSYSTEM_MEDIA,
	},
	{
		.ucg_id = CLK_MEDIA_UCG2,
		.first_chan_id = CLK_MEDIA_UCG2_GPU_SYS,
		.base_addr = 0x1320140,
		.names = { "gpu_sys_clk", "gpu_mem_clk", "gpu_core_clk" },
		.parent_name = "media_pll2",
		.subsystem = SUBSYSTEM_MEDIA,
	},
	{
		.ucg_id = CLK_MEDIA_UCG3,
		.first_chan_id = CLK_MEDIA_UCG3_MIPI_RX_REF,
		.base_addr = 0x13201c0,
		.names = { "mipi_rx_refclk", "mipi_rx0_cfg_clk",
			   "mipi_rx1_cfg_clk", "mipi_tx_ref_clk",
			   "mipi_tx_cfg_clk", "cmos0_clk", "cmos1_clk",
			   "mipi_txclkesc", "vpu_clk" },
		.parent_name = "media_pll3",
		.subsystem = SUBSYSTEM_MEDIA,
	},
	{
		.ucg_id = CLK_HSP_UCG0,
		.first_chan_id = CLK_HSP_UCG0_SYS,
		.base_addr = 0x10410000,
		.names = { "hsp_sys_clk", "hsp_dma_clk", "hsp_ctr_clk",
			   "spram_clk", "emac0_clk", "emac1_clk", "usb0_clk",
			   "usb1_clk", "nfc_clk", "pdma2_clk", "sdmmc0_clk",
			   "sdmmc1_clk", "qspi_clk" },
		.parent_name = "hsp_refmux0",
		.subsystem = SUBSYSTEM_HSP,
	},
	{
		.ucg_id = CLK_HSP_UCG1,
		.first_chan_id = CLK_HSP_UCG1_SDMMC0_XIN,
		.base_addr = 0x10420000,
		.names = { "sdmmc0_xin_clk", "sdmmc1_xin_clk", "nfc_clk_flash",
			   "qspi_ext_clk", "ust_clk" },
		.parent_name = "hsp_refmux1",
		.subsystem = SUBSYSTEM_HSP,
	},
	{
		.ucg_id = CLK_HSP_UCG2,
		.first_chan_id = CLK_HSP_UCG2_EMAC0_1588,
		.base_addr = 0x10430000,
		.names = { "emac0_clk_1588", "emac0_rgmii_txc",
			   "emac1_clk_1588", "emac1_rgmii_txc" },
		.parent_name = "hsp_refmux2",
		.subsystem = SUBSYSTEM_HSP,
	},
	{
		.ucg_id = CLK_HSP_UCG3,
		.first_chan_id = CLK_HSP_UCG3_USB0_REF_ALT,
		.base_addr = 0x10440000,
		.names = { "usb0_ref_alt_clk", "usb0_suspend_clk",
			   "usb1_ref_alt_clk", "usb1_suspend_clk" },
		.parent_name = "hsp_refmux3",
		.subsystem = SUBSYSTEM_HSP,
	},
	{
		.ucg_id = CLK_LSP0_UCG0,
		.first_chan_id = CLK_LSP0_UCG0_SYS,
		.base_addr = 0x1690000,
		.names = { "lsp0_sys_clk", "uart3_clk", "uart1_clk",
			   "uart2_clk", "spi0_clk", "i2c0_clk", "gpio0_dbclk" },
		.parent_name = "lsperiph0_pll",
		.subsystem = SUBSYSTEM_LSP0,
	},
	{
		.ucg_id = CLK_LSP1_UCG0,
		.first_chan_id = CLK_LSP1_UCG0_SYS,
		.base_addr = 0x17c0000,
		.names = { "lsp1_sys_clk", "i2c1_clk", "i2c2_clk", "i2c3_clk",
			   "gpio1_dbclk", "ssi1_clk", "uart0_clk",
			   "timers0_clk", "pwm0_clk", "wdt1_clk" },
		.parent_name = "lsperiph1_pll",
		.subsystem = SUBSYSTEM_LSP1,
	},
	{
		.ucg_id = CLK_LSP1_UCG_I2S,
		.first_chan_id = CLK_LSP1_UCG_I2S_I2S0,
		.base_addr = 0x17d0000,
		.names = { "lsp1_i2s_clk" },
		.parent_name = "lsperiph1_refmux_i2s",
		.subsystem = SUBSYSTEM_LSP1_I2S_UCG,
	},
};

static struct mcom03_pll *mcom03_clk_pll_find_by_id(u32 id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(pll_clocks); i++) {
		if (pll_clocks[i].clk.id == id)
			return &pll_clocks[i];
	}

	return NULL;
}

static struct mcom03_refmux *mcom03_clk_refmux_find_by_id(u32 id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(refmux_clocks); i++) {
		if (refmux_clocks[i].clk_id == id)
			return &refmux_clocks[i];
	}

	return NULL;
}

static struct mcom03_ucg *mcom03_clk_ucg_find_by_id(u32 id)
{
	int i;

	id &= ~0xf;
	for (i = 0; i < ARRAY_SIZE(ucg_clocks); i++) {
		if (ucg_clocks[i].first_chan_id == id)
			return &ucg_clocks[i];
	}

	return NULL;
}

static struct mcom03_ucg *mcom03_clk_ucg_find_by_ucg_id(u32 ucg_id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ucg_clocks); i++) {
		if (ucg_clocks[i].ucg_id == ucg_id)
			return &ucg_clocks[i];
	}

	return NULL;
}

static enum mcom03_clk_type mcom03_clk_find_by_id(u32 id,
						  struct clk **clkp)
{
	if (id >= CLK_FIRST_PLL && id <= CLK_LAST_PLL) {
		struct mcom03_pll *pll = mcom03_clk_pll_find_by_id(id);

		if (pll) {
			*clkp = &pll->clk;
			return PLL;
		}
	} else if (id >= CLK_FIRST_REFMUX && id <= CLK_LAST_REFMUX) {
		return REFMUX;
	} else if (id > CLK_LAST_REFMUX) {
		struct mcom03_ucg *ucg = mcom03_clk_ucg_find_by_id(id);

		if (ucg) {
			*clkp = &ucg->clks[id & 0xf];
			return UCG;
		}
	}

	return UNKNOWN;
}

static ulong mcom03_clk_pll_get_rate(struct clk *clk)
{
	struct mcom03_pll *pll = container_of(clk, struct mcom03_pll, clk);
	ulong parent_rate = clk_get_parent_rate(clk);
	u32 reg = readl(pll->iobase);
	u8 nr;
	u16 nf;
	u8 od;

	if (reg & PLL_SEL) {
		if (!(reg & PLL_LOCK)) {
			log_warning("%s: PLL is not locked\n",
				    dev_get_uclass_name(clk->dev));
			return 0;
		} else if (reg & PLL_MAN) {
			nf = FIELD_GET(PLL_NF, reg) + 1;
			nr = FIELD_GET(PLL_NR, reg) + 1;
			od = FIELD_GET(PLL_OD, reg) + 1;
			return parent_rate * nf / nr / od;
		}
	}

	return parent_rate * (FIELD_GET(PLL_SEL, reg) + 1);
}

static long _pll_round_rate(ulong rate, ulong parent_rate, bool update_param,
			    u8 max_nr, bool *bypass, u16 *_nf, u8 *_nr, u8 *_od)
{
	const u8 od_list[] = {16, 14, 12, 10, 8, 6, 4, 2, 1};
	ulong closest_freq = parent_rate;
	ulong vco;
	ulong freq;
	ulong nf_frac;
	ulong nf;
	u16 closest_nf = 0;
	u8 nr, od, closest_nr = 0, closest_od = 0;
	int i;

	if (!parent_rate)
		return -EINVAL;

	/* Maximum PLL output rate is PLL_VCO_MAX_FREQ */
	rate = min(rate, PLL_VCO_MAX_FREQ);

	/* If bypass is used then pll_rate = parent_rate
	 * If bypass is not used then:
	 * pll_rate = VCO / OD
	 * where
	 * VCO = parent_rate * NF / NR
	 * VCO must be in range [720 MHz ... 3600 MHz]
	 * NR should be minimal to jitter minimization.
	 * VCO and OD should be maximum as possible.
	 */
	for (nr = 1; nr <= max_nr; nr++) {
		for (i = 0; i < ARRAY_SIZE(od_list); i++) {
			od = od_list[i];
			nf = rate * nr * od;
			nf_frac = do_div(nf, (u32)parent_rate);
			vco = parent_rate * nf / nr;
			if (nf == 0 || vco < PLL_VCO_MIN_FREQ) {
				/* Current od is not suitable. Break the cycle
				 * since all subsequent ods in the list are less
				 * than the current one. */
				break;
			} else if (nf > PLL_NF_MAX || vco > PLL_VCO_MAX_FREQ) {
				/* Try next OD */
				continue;
			}

			if (nf_frac == 0) {
				/* Found coefficients to get rate frequency */
				if (update_param) {
					*_nf = nf;
					*_nr = nr;
					*_od = od;
					*bypass = false;
				}

				/* Use DIV_ROUND_CLOSEST() because of
				 * rounding errors */
				return DIV_ROUND_CLOSEST(vco, od);
			}
			freq = vco / od;
			if (freq > closest_freq) {
				closest_freq = freq;
				closest_nf = nf;
				closest_nr = nr;
				closest_od = od;
			}
		}
	}
	if (update_param) {
		*_nf = closest_nf;
		*_nr = closest_nr;
		*_od = closest_od;
		*bypass = (closest_freq == parent_rate);
	}

	return closest_freq;
}

static ulong mcom03_clk_pll_round_rate(struct clk *clk, ulong rate)
{
	ulong parent_rate = clk_get_parent_rate(clk);
	struct mcom03_pll *pll = container_of(clk, struct mcom03_pll, clk);

	return _pll_round_rate(rate, parent_rate, false, pll->max_nr,
			       NULL, NULL, NULL, NULL);
}

static bool mcom03_clk_ucg_is_enabled(struct clk *clk)
{
	u32 id = clk->id & 0xf;
	struct mcom03_ucg *ucg = container_of(clk, struct mcom03_ucg, clks[id]);
	u32 value = readl(ucg->iobase + id * sizeof(u32));

	return value & CLK_EN;
}

/*
 * Save current UCGs frequencies to their caches and enable bypass for all
 * enabled channels.
 */
static void mcom03_clk_save_ucg_rates(u8 *ucg_ids, u8 ucg_id_len)
{
	struct mcom03_ucg *ucg;
	u16 mask;

	for (u8 i = 0; i < ucg_id_len; i++) {
		mask = 0;
		ucg = mcom03_clk_ucg_find_by_ucg_id(ucg_ids[i]);
		for (int j = 0; j < 16; j++) {
			if (!clk_valid(&ucg->clks[j]))
				continue;

			ucg->clks[j].rate = clk_get_rate(&ucg->clks[j]);
			if (mcom03_clk_ucg_is_enabled(&ucg->clks[j]))
				mask |= BIT(j);
		}
		writel(mask, ucg->iobase + BP_CTR_REG);
	}
}

/*
 * Restore saved UCGs frequencies and disable bypass.
 */
static void mcom03_clk_restore_ucg_rates(u8 *ucg_ids, u8 ucg_id_len)
{
	struct mcom03_ucg *ucg;

	for (u8 i = 0; i < ucg_id_len; i++) {
		ucg = mcom03_clk_ucg_find_by_ucg_id(ucg_ids[i]);
		for (int j = 0; j < 16; j++) {
			if (!clk_valid(&ucg->clks[j]))
				continue;

			clk_set_rate(&ucg->clks[j], ucg->clks[j].rate);
			ucg->clks[j].rate = 0;
		}
		/* Disable bypass for all channels. This is safe because in
		 * mcom03_clk_save_ucg_rates() we saved correct channel
		 * rate (xti_clk rate if bypass was enabled). This rate
		 * restored in current function by divider and now we
		 * can disable bypass.
		 */
		writel(0, ucg->iobase + BP_CTR_REG);
	}
}

static ulong mcom03_clk_pll_set_rate(struct clk *clk, ulong rate)
{
	ulong parent_rate = clk_get_parent_rate(clk);
	struct mcom03_pll *pll = container_of(clk, struct mcom03_pll, clk);
	u32 reg = readl(pll->iobase);
	bool bypass;
	u16 nf;
	u8 nr, od;
	long res = _pll_round_rate(rate, parent_rate, true, pll->max_nr,
				   &bypass, &nf, &nr, &od);

	if (res < 0)
		return (ulong)res;

	/* If we increase PLL output rate then new frequency will pass to UCGs
	 * where divider calculated to old frequency.
	 * Example: MEDIA_PLL0 was setup to 27 MHz, after this PLL placed UCG0
	 * where channel 0 is media_sys_aclk clock. Maximum rate for
	 * media_sys_aclk is 250 MHz. In beginning divider to media_sys_aclk is
	 * equal to 1. If we setup MEDIA_PLL0 to 1 GHz then 1 GHz will pass to
	 * media_sys_aclk and this will cause a hang.
	 *
	 * Before PLL rate change we need to save current UCGs channels
	 * frequencies and turn on bypass for all enabled channels. After
	 * rate changing we need to resetup all channels to old frequencies and
	 * turn off bypass.
	 */
	mcom03_clk_save_ucg_rates(pll->ucg_ids, pll->ucg_id_len);
	if (bypass)
		reg = 0;
	else
		reg = FIELD_PREP(PLL_NF, nf - 1) |
		      FIELD_PREP(PLL_NR, nr - 1) |
		      FIELD_PREP(PLL_OD, od - 1) |
		      FIELD_PREP(PLL_SEL, 1) | PLL_MAN;

	if ((readl(pll->iobase) & ~PLL_LOCK) == reg)
		return rate;

	writel(reg, pll->iobase);

	/* PLL resets LOCK bit in few clock cycles after
	 * new PLL settings are set
	 */
	udelay(1);

	if (!bypass) {
		if (readl_poll_timeout(pll->iobase, reg, reg & PLL_LOCK, 1000)) {
			log_err("Failed to lock PLL %s\n",
				dev_get_uclass_name(clk->dev));
			return (ulong)-ETIMEDOUT;
		}
	}

	/* Clear cached rate to make possible read correct parrent rate by UCGs
	 * in mcom03_clk_restore_ucg_rates() */
	clk->rate = 0;
	mcom03_clk_restore_ucg_rates(pll->ucg_ids, pll->ucg_id_len);

	return (ulong)res;
}

static ulong mcom03_clk_ucg_round_rate(struct clk *clk, ulong rate)
{
	ulong parent_rate = clk_get_parent_rate(clk);
	u32 id = clk->id & 0xf;
	struct mcom03_ucg *ucg = container_of(clk, struct mcom03_ucg, clks[id]);
	u32 div;
	bool round_up = ucg->round_up_mask & BIT(id);

	if (ucg->fixed_freq_mask & BIT(id))
		return (ulong)-ENOSYS;

	div = round_up ? parent_rate / rate : DIV_ROUND_UP(parent_rate, rate);
	div = min(div, UCG_MAX_DIVIDER);

	return round_up ? parent_rate / div : DIV_ROUND_UP(parent_rate, div);
}

static ulong mcom03_clk_ucg_get_rate(struct clk *clk)
{
	u32 id = clk->id & 0xf;
	struct mcom03_clk_plat *plat = dev_get_priv(clk->dev);
	struct mcom03_ucg *ucg = container_of(clk, struct mcom03_ucg, clks[id]);
	ulong parent_rate = clk_get_parent_rate(clk);
	u32 reg = readl(ucg->iobase + id * sizeof(u32));
	u32 div = FIELD_GET(DIV_COEFF, reg);
	bool bp = readl(ucg->iobase + BP_CTR_REG) & BIT(id);

	if (!div)
		div = 1;

	return bp ? clk_get_rate(&plat->xti_clk) :
			DIV_ROUND_UP(parent_rate, div);
}

static ulong mcom03_clk_ucg_set_rate(struct clk *clk, ulong rate)
{
	u32 id = clk->id & 0xf;
	struct mcom03_ucg *ucg = container_of(clk, struct mcom03_ucg, clks[id]);
	ulong parent_rate = clk_get_parent_rate(clk);
	bool round_up = ucg->round_up_mask & BIT(id);
	u32 div = round_up ? parent_rate / rate :
			DIV_ROUND_UP(parent_rate, rate);
	u32 value;
	u32 bp;
	int is_enabled;
	ulong ret;

	if (ucg->fixed_freq_mask & BIT(id))
		return (ulong)-ENOSYS;

	value = readl(ucg->iobase + id * sizeof(u32));
	div = min_t(u32, div, UCG_MAX_DIVIDER);
	div = max_t(u32, div, 1);
	/* Check for divider already correct */
	if (FIELD_GET(DIV_COEFF, value) == div)
		return DIV_ROUND_UP(parent_rate, div);

	is_enabled = value & CLK_EN;
	bp = readl(ucg->iobase + BP_CTR_REG);
	/* Use bypass mode if channel is enabled */
	if (is_enabled)
		writel(bp | BIT(id), ucg->iobase + BP_CTR_REG);

	value &= ~DIV_COEFF;
	value |= FIELD_PREP(DIV_COEFF, div);
	writel(value, ucg->iobase + id * sizeof(u32));
	ret = DIV_ROUND_UP(parent_rate, div);
	if (readl_poll_timeout(ucg->iobase + id * sizeof(u32), value,
			       value & DIV_LOCK, 10000)) {
		log_err("%s: Failed to lock divider\n",
			clk_hw_get_name(clk));
		ret = (ulong)-ETIMEDOUT;
	}

	if (is_enabled)
		writel(bp & ~BIT(id), ucg->iobase + BP_CTR_REG);

	return ret;
}

static int mcom03_clk_ucg_enable(struct clk *clk)
{
	u32 id = clk->id & 0xf;
	struct mcom03_ucg *ucg = container_of(clk, struct mcom03_ucg, clks[id]);
	u32 value = readl(ucg->iobase + id * sizeof(u32));

	value |= CLK_EN;
	value &= ~LPI_EN;
	writel(value, ucg->iobase + id * sizeof(u32));

	return readl_poll_timeout(ucg->iobase + id * sizeof(u32), value,
				  (value & Q_FSM_STATE) == Q_RUN, 10000);
}

static int mcom03_clk_ucg_disable(struct clk *clk)
{
	u32 id = clk->id & 0xf;
	struct mcom03_ucg *ucg = container_of(clk, struct mcom03_ucg, clks[id]);
	u32 value = readl(ucg->iobase + id * sizeof(u32));

	value &= ~(LPI_EN | CLK_EN);
	writel(value, ucg->iobase + id * sizeof(u32));

	return readl_poll_timeout(ucg->iobase + id * sizeof(u32), value,
				  (value & Q_FSM_STATE) == Q_STOPPED, 10000);
}

static bool mcom03_is_subsystem_available(struct mcom03_clk_plat *plat,
					  u32 subsystem)
{
	u32 value;

	if (subsystem == SUBSYSTEM_LSP1_I2S_UCG) {
		/* Check LSP1 subsystem before read register from LSP1 URB */
		if (!mcom03_is_subsystem_available(plat, SUBSYSTEM_LSP1))
			return false;

		value = readl(plat->i2s_ucg_rstn_pstatus);
	} else {
		if (regmap_read(plat->serv_urb, subsystem * 8 + 4, &value))
			return false;
	}

	return value == PP_ON;
}

static bool mcom03_is_clk_available(struct clk *clk, enum mcom03_clk_type clk_type)
{
	struct mcom03_clk_plat *plat = dev_get_priv(clk->dev);
	struct mcom03_ucg *ucg;
	struct mcom03_pll *pll;
	u32 subsystem;

	switch (clk_type) {
	case PLL:
		pll = container_of(clk, struct mcom03_pll, clk);
		subsystem = pll->subsystem;
		break;
	case UCG:
		ucg = container_of(clk, struct mcom03_ucg, clks[clk->id & 0xf]);
		subsystem = ucg->subsystem;
		break;
	case REFMUX:
		return true;
	default:
		return false;
	}

	return mcom03_is_subsystem_available(plat, subsystem);
}

static ulong mcom03_clk_round_rate(struct clk *clk, ulong rate)
{
	enum mcom03_clk_type clk_type;

	clk_type = mcom03_clk_find_by_id(clk->id, &clk);
	if (!mcom03_is_clk_available(clk, clk_type))
		return 0;

	switch (clk_type) {
	case PLL:
		return mcom03_clk_pll_round_rate(clk, rate);
	case UCG:
		return mcom03_clk_ucg_round_rate(clk, rate);
	default:
		return (ulong)-EINVAL;
	}
}

static ulong mcom03_clk_get_rate(struct clk *clk)
{
	enum mcom03_clk_type clk_type;

	clk_type = mcom03_clk_find_by_id(clk->id, &clk);
	if (!mcom03_is_clk_available(clk, clk_type))
		return 0;

	switch (clk_type) {
	case PLL:
		return mcom03_clk_pll_get_rate(clk);
	case UCG:
		return mcom03_clk_ucg_get_rate(clk);
	case REFMUX:
		return clk_get_parent_rate(clk);
	default:
		return -EINVAL;
	}
}

static ulong mcom03_clk_set_rate(struct clk *clk, ulong rate)
{
	enum mcom03_clk_type clk_type;

	clk_type = mcom03_clk_find_by_id(clk->id, &clk);
	if (!mcom03_is_clk_available(clk, clk_type))
		return 0;

	switch (clk_type) {
	case PLL:
		return mcom03_clk_pll_set_rate(clk, rate);
	case UCG:
		return mcom03_clk_ucg_set_rate(clk, rate);
	default:
		return (ulong)-EINVAL;
	}
}

static int mcom03_clk_enable(struct clk *clk)
{
	enum mcom03_clk_type clk_type;

	clk_type = mcom03_clk_find_by_id(clk->id, &clk);
	if (!mcom03_is_clk_available(clk, clk_type))
		return -EPERM;

	switch (clk_type) {
	case PLL:
	case REFMUX:
		return 0;
	case UCG:
		return mcom03_clk_ucg_enable(clk);
	default:
		return -EINVAL;
	}
}

static int mcom03_clk_disable(struct clk *clk)
{
	enum mcom03_clk_type clk_type;

	clk_type = mcom03_clk_find_by_id(clk->id, &clk);
	if (!mcom03_is_clk_available(clk, clk_type))
		return -EPERM;

	switch (clk_type) {
	case PLL:
	case REFMUX:
		return 0;
	case UCG:
		return mcom03_clk_ucg_disable(clk);
	default:
		return -EINVAL;
	}
}

static int mcom03_clk_set_parent(struct clk *clk, struct clk *parent)
{
	struct mcom03_clk_plat *plat = dev_get_priv(clk->dev);
	struct mcom03_refmux *refmux;
	enum mcom03_clk_type clk_type;

	clk_type = mcom03_clk_find_by_id(clk->id, &clk);

	switch (clk_type) {
	case REFMUX:
		refmux = mcom03_clk_refmux_find_by_id(clk->id);
		if (!mcom03_is_subsystem_available(plat, refmux->subsystem))
			return -EPERM;

		mcom03_clk_save_ucg_rates(&refmux->ucg_id, 1);
		clk_mux_ops.set_parent(clk, parent);
		mcom03_clk_restore_ucg_rates(&refmux->ucg_id, 1);
		return 0;
	default:
		return -EINVAL;
	}
}

static struct clk_ops mcom03_clk_ops = {
	.round_rate = mcom03_clk_round_rate,
	.get_rate = mcom03_clk_get_rate,
	.set_rate = mcom03_clk_set_rate,
	.enable = mcom03_clk_enable,
	.disable = mcom03_clk_disable,
	.set_parent = mcom03_clk_set_parent,
};

static int mcom03_clk_read_settings(struct udevice *dev,
				    enum property_type prop_type)
{
	char *prop;
	struct mcom03_pll *pll;
	struct mcom03_ucg *ucg;
	u32 id;
	u32 value;
	int size;

	switch (prop_type) {
	case MAX_NR:
		prop = "elvees,pll-max-nr";
		break;
	case UCG_FIXED_FREQ_MASK:
		prop = "elvees,ucg-fixed-freq-mask";
		break;
	case UCG_ROUND_UP_MASK:
		prop = "elvees,ucg-round-up-mask";
		break;
	default:
		return -EINVAL;
	}

	size = dev_read_size(dev, prop);
	if (size > 0) {
		if (size & 0x7) {
			log_err("%s: Wrong size of %s property\n",
				dev->name, prop);
			return -EINVAL;
		}
		size /= sizeof(u32);
		for (int i = 0; i < size; i += 2) {
			dev_read_u32_index(dev, prop, i, &id);
			dev_read_u32_index(dev, prop, i + 1, &value);
			if (prop_type == MAX_NR && value > 16) {
				log_err("%s: Wrong max-nr (%d)\n",
					dev->name, value);
				return -EINVAL;
			}
			if (prop_type == MAX_NR) {
				pll = mcom03_clk_pll_find_by_id(id);
				if (!pll) {
					log_err("%s: Unsupported PLL id (%#x) in %s property\n",
						dev->name, id, prop);
					return -EINVAL;
				}
				pll->max_nr = value;
			} else if (prop_type == UCG_FIXED_FREQ_MASK ||
				   prop_type == UCG_ROUND_UP_MASK) {
				ucg = mcom03_clk_ucg_find_by_ucg_id(id);
				if (!ucg) {
					log_err("%s: Unsupported UCG id (%#x) in %s property\n",
						dev->name, id, prop);
					return -EINVAL;
				}
				if (prop_type == UCG_FIXED_FREQ_MASK)
					ucg->fixed_freq_mask = value;
				else
					ucg->round_up_mask = value;
			}
		}
	}

	return 0;
}

static int mcom03_clk_of_to_plat(struct udevice *dev)
{
	struct mcom03_clk_plat *plat = dev_get_plat(dev);
	struct clk clk125;
	struct clk i2s_sclk_in;
	int res;
	int i;

	res = clk_get_by_index(dev, 0, &plat->xti_clk);
	if (res < 0) {
		log_err("%s: Failed to find parent clock\n", dev->name);
		return -EINVAL;
	}

	res = clk_get_by_index(dev, 1, &clk125);
	if (res < 0) {
		log_err("%s: Failed to find parent clock\n", dev->name);
		return -EINVAL;
	}

	res = clk_get_by_index(dev, 2, &i2s_sclk_in);
	if (res < 0) {
		log_err("%s: Failed to find parent clock\n", dev->name);
		return -EINVAL;
	}

	res = mcom03_clk_read_settings(dev, MAX_NR);
	if (res)
		return res;

	res = mcom03_clk_read_settings(dev, UCG_FIXED_FREQ_MASK);
	if (res)
		return res;

	res = mcom03_clk_read_settings(dev, UCG_ROUND_UP_MASK);
	if (res)
		return res;

	plat->i2s_ucg_rstn_pstatus = map_sysmem(LSP1_I2S_UCG_RSTN_PSTATUS, 0x4);
	plat->serv_urb = syscon_regmap_lookup_by_phandle(dev,
							 "elvees,service-urb");
	if (IS_ERR(plat->serv_urb))
		return PTR_ERR(plat->serv_urb);

	for (i = 0; i < ARRAY_SIZE(pll_clocks); i++) {
		pll_clocks[i].iobase = map_sysmem(pll_clocks[i].base_addr,
						  PLL_SIZE);
		res = clk_register(&pll_clocks[i].clk, dev->driver->name,
				   pll_clocks[i].name,
				   plat->xti_clk.dev->name);
		if (res)
			log_err("%s: Failed to register %s (%d)\n",
				dev->name, pll_clocks[i].name, res);

		dev_set_priv(pll_clocks[i].clk.dev, plat);
	}

	for (i = 0; i < ARRAY_SIZE(refmux_clocks); i++) {
		struct clk *clk;

		if (refmux_clocks[i].clk_id >= CLK_HSP_REFMUX0 &&
		    refmux_clocks[i].clk_id <= CLK_HSP_REFMUX3)
			refmux_clocks[i].parent_names[1] = clk125.dev->name;
		else
			refmux_clocks[i].parent_names[1] = i2s_sclk_in.dev->name;

		clk = clk_register_mux(NULL, refmux_clocks[i].name,
				       refmux_clocks[i].parent_names,
				       refmux_clocks[i].num_parents,
				       CLK_SET_RATE_NO_REPARENT | CLK_GET_RATE_NOCACHE,
				       map_sysmem(refmux_clocks[i].base_addr, 4),
				       refmux_clocks[i].shift,
				       refmux_clocks[i].width, 0);
		if (IS_ERR(clk))
			log_err("%s: Failed to register %s (%ld)\n",
				dev->name, refmux_clocks[i].name, PTR_ERR(clk));

		/* Dirty hack because we want to use clk_ops from current
		 * driver, not from clk_mux.
		 */
		clk->dev->driver = dev->driver;

		dev_set_priv(clk->dev, plat);
		clk->id = refmux_clocks[i].clk_id;
	}

	for (i = 0; i < ARRAY_SIZE(ucg_clocks); i++) {
		int chan;

		ucg_clocks[i].iobase = map_sysmem(ucg_clocks[i].base_addr,
						  UCG_SIZE);
		for (chan = 0; chan < ARRAY_SIZE(ucg_clocks[i].names); chan++) {
			/* names[chan] can be NULL for channel 0 of JESD UCGs */
			if (!ucg_clocks[i].names[chan])
				continue;

			ucg_clocks[i].clks[chan].id = ucg_clocks[i].first_chan_id +
						      chan;
			res = clk_register(&ucg_clocks[i].clks[chan],
					   dev->driver->name,
					   ucg_clocks[i].names[chan],
					   ucg_clocks[i].parent_name);
			if (res)
				log_err("%s: Failed to register %s\n",
					dev->name,
					ucg_clocks[i].names[chan]);

			dev_set_priv(ucg_clocks[i].clks[chan].dev, plat);
		}
	}

	return 0;
}

static const struct udevice_id mcom03_clk_ids[] = {
	{ .compatible = "elvees,mcom03-clk" },
	{}
};

U_BOOT_DRIVER(mcom03_clk) = {
	.name		= "mcom03_clk",
	.id		= UCLASS_CLK,
	.of_match	= mcom03_clk_ids,
	.ops		= &mcom03_clk_ops,
	.of_to_plat	= mcom03_clk_of_to_plat,
	.plat_auto	= sizeof(struct mcom03_clk_plat),
	.flags		= DM_FLAG_PRE_RELOC,
};

static int mcom03_clk_fixed_of_to_plat(struct udevice *dev)
{
	struct clk_fixed_rate *plat = dev_get_plat(dev);
	u32 id;
	int res;

	res = dev_read_u32(dev, "elvees,id", &id);
	if (res) {
		log_err("%s: failed to read elvees,id (%d)\n", dev->name, res);
		return res;
	}
	plat->clk.id = id;
	clk_fixed_rate_ofdata_to_plat_(dev, plat);

	return 0;
}

static const struct udevice_id mcom03_clk_fixed_ids[] = {
	{ .compatible = "elvees,mcom03-clk-fixed" },
	{}
};

/* Common driver with compatible "fixed-clock" always uses zero for clock ID.
 * This cause erorrs if fixed-clock declared several times and one of them used
 * for calling clk_set_parent. In this driver used unique clock ID.
 */
U_BOOT_DRIVER(mcom03_clk_fixed) = {
	.name		= "mcom03_clk_fixed",
	.id		= UCLASS_CLK,
	.of_match	= mcom03_clk_fixed_ids,
	.ops		= &clk_fixed_rate_ops,
	.of_to_plat	= mcom03_clk_fixed_of_to_plat,
	.plat_auto	= sizeof(struct clk_fixed_rate),
	.flags		= DM_FLAG_PRE_RELOC,
};
