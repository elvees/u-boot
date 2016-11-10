/*
 * Copyright 2012-2013 Henrik Nordstrom <henrik@henriknordstrom.net>
 * Copyright 2013 Luke Kenneth Casson Leighton <lkcl@lkcl.net>
 * Copyright 2007-2011 Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * Copyright 2015-2016 ELVEES NeoTek JSC, <www.elvees-nt.com>
 * Copyright 2017 RnD Center "ELVEES", OJSC
 *
 * Some board initialization for the ELVEES MCom-compatible board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <mmc.h>
#ifdef CONFIG_SPL_BUILD
#include <spl.h>
#include <asm/arch/bootrom.h>
#include <asm/arch/ddr.h>
#endif
#include <asm/arch/clock.h>
#include <asm/arch/regs.h>
#include <asm/arch/sdmmc.h>
#include <asm/io.h>
#include <linux/kernel.h>

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	return 0;
}

static bool is_ddrmc_active(int ddrmc_id)
{
	cmctr_t *CMCTR = (cmctr_t *)CMCTR_BASE;
	ddrmc_t *DDRMC;
	bool clk_en;

	uint32_t reg = CMCTR->GATE_CORE_CTR;

	switch (ddrmc_id) {
	case 0:
		DDRMC = (ddrmc_t *)DDRMC0_BASE;
		clk_en = (reg >> 1) & 1;
		break;
	case 1:
		DDRMC = (ddrmc_t *)DDRMC1_BASE;
		clk_en = (reg >> 2) & 1;
		break;
	default:
		puts("is_ddrmc_active: Invalid argument\n");
		return 0;
	}

	/* If DDRMC clock disabled then DDRMC is not active.
	 * If clock enabled we should additionally check
	 * STAT register.
	 */

	if (clk_en && (DDRMC->STAT == 0x1))
		return 1;

	return 0;
}

void dram_init_banksize(void)
{
	/* DDR controller #0 is active since TPL uses it */
	gd->bd->bi_dram[0].start = PHYS_SDRAM_0;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_0_SIZE;

	/* If DDR controller #1 is active increase
	 * available for Linux memory size
	 */
	if (is_ddrmc_active(1)) {
		gd->bd->bi_dram[1].start = PHYS_SDRAM_1;
		gd->bd->bi_dram[1].size = PHYS_SDRAM_1_SIZE;
	} else {
		gd->bd->bi_dram[1].start = 0;
		gd->bd->bi_dram[1].size = 0;
	}
}

#ifdef CONFIG_SPL_BUILD
/* Set parameters for  Nanya NT5CB256M8FN-DI SDRAM */
static int set_nt5cb256m8fndi_cfg(struct ddr_cfg *cfg, int tck)
{
	if (!cfg) {
		printf("Invalid pointer to DDR configuration\n");
		return -EINVAL;
	}

	if (!tck) {
		printf("Invalid clock period\n");
		return -EINVAL;
	}

	cfg->common.ranks = MCOM_SDRAM_ONE_RANK;
	cfg->common.banks = 8;
	cfg->common.columns = 1024;
	cfg->common.rows = 32768;
	cfg->common.bl = 8;
	cfg->common.cl = 7;
	cfg->common.cwl = 6;
	cfg->common.twr = to_clocks(15000, tck);
	cfg->common.tfaw = to_clocks(30000, tck);
	cfg->common.tras = to_clocks(35000, tck);
	cfg->common.tras_max = to_clocks(9 * 7800000, tck);
	cfg->common.trc = to_clocks(48750, tck);
	cfg->common.txp = to_clocks(max(3 * tck, 6000), tck);
	cfg->common.trtp = to_clocks(max(4 * tck, 7500), tck);
	cfg->common.twtr = to_clocks(max(4 * tck, 7500), tck);
	cfg->common.trcd = to_clocks(13750, tck);
	cfg->common.trrd = to_clocks(max(4 * tck, 6000), tck);
	cfg->common.tccd = 4;
	cfg->common.tcke = to_clocks(max(3 * tck, 5000), tck);
	cfg->common.tckesr = cfg->common.tcke + 1;
	cfg->common.tzqcs = to_clocks(max(64 * tck, 80000), tck);
	cfg->common.trefi = to_clocks(7800000, tck);
	cfg->ddr3.trp = to_clocks(13750, tck);
	cfg->ddr3.txpdll = to_clocks(max(10 * tck, 24000), tck);
	cfg->ddr3.tmrd = 4;
	cfg->ddr3.tmod = to_clocks(max(12 * tck, 15000), tck);
	cfg->ddr3.tcksre = to_clocks(max(5 * tck, 10000), tck);
	cfg->ddr3.tcksrx = to_clocks(max(5 * tck, 10000), tck);
	cfg->ddr3.tzqoper = to_clocks(max(256 * tck, 320000), tck);
	cfg->ddr3.trfc = to_clocks(160000, tck);
	cfg->ddr3.tdllk = 512;
	cfg->ddr3.txsdll = cfg->ddr3.tdllk;
	cfg->ddr3.txs = max(5, to_clocks(10000, tck) + (int)cfg->ddr3.trfc);

	return 0;
}
#endif

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_0_SIZE;
#ifdef CONFIG_SPL_BUILD
	struct ddr_cfg cfg[2];
	struct ddr_freq freq;
	int i, ret;

	freq.xti_freq = XTI_FREQ;
	freq.cpll_mult = CPLL_VALUE;
	freq.ddr0_div = DIV_DDR0_CTR_VALUE;
	freq.ddr1_div = DIV_DDR1_CTR_VALUE;

	for (i = 0; i < 2; i++) {
		ret = set_nt5cb256m8fndi_cfg(&cfg[i],
					     ddr_get_clock_period(i, &freq));
		if (ret)
			return ret;
		cfg[i].ctl_id = i;
		cfg[i].type = MCOM_SDRAM_TYPE_DDR3;
	}

	timer_init();

	puts("DDR controllers init started\n");

	u32 status = mcom_ddr_init(&cfg[0], &cfg[1], &freq);

	if (ddr_getrc(status, 0))
		puts("DDR controller #0 init failed\n");
	else
		puts("DDR controller #0 init done\n");

	if (ddr_getrc(status, 1))
		puts("DDR controller #1 init failed\n");
	else
		puts("DDR controller #1 init done\n");

	return status;
#else
	return 0;
#endif
}

#ifdef CONFIG_GENERIC_MMC
int board_mmc_init(bd_t *bis)
{
	return sdmmc_init(SDMMC0_BASE, SPLL_FREQ);
}
#endif

#ifdef CONFIG_SPL_BUILD
u32 spl_boot_device(void)
{
	smctr_t *SMCTR = (smctr_t *)SMCTR_BASE;

	switch (SMCTR->BOOT) {
	case SMCTR_BOOT_SPI0:
		return BOOT_DEVICE_SPI;
	case SMCTR_BOOT_SDMMC0:
		return BOOT_DEVICE_MMC1;
	}

	return BOOT_DEVICE_NONE;
}

u32 spl_boot_mode(void)
{
	return MMCSD_MODE_RAW;
}
#endif
