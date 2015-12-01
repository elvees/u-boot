/*
 * (C) Copyright 2012-2013 Henrik Nordstrom <henrik@henriknordstrom.net>
 * (C) Copyright 2013 Luke Kenneth Casson Leighton <lkcl@lkcl.net>
 *
 * (C) Copyright 2007-2011
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * (C) Copyright 2015
 * ELVEES NeoTek CJSC, <www.elvees-nt.com>
 *
 * Dmitriy Zagrebin <dzagrebin@elvees.com>
 *
 * Some board init for the Elvees SBC-DBG board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <mmc.h>
#ifdef CONFIG_SPL_BUILD
#include <asm/arch/bootrom.h>
#endif
#include <asm/arch/clock.h>
#include <asm/arch/ddr_cfg.h>
#include <asm/arch/regs.h>
#include <asm/arch/sdmmc.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	return 0;
}

void set_nt5cb256m8fn_di_cfg(ddr3_t *ddr3_mem)
{
	ddr3_mem->misc0.full_reg = 0;
	ddr3_mem->misc0.burst_rdwr = 4;
	ddr3_mem->misc0.ddr3_type = 1;
	ddr3_mem->misc0.lpddr2_type = 0;
	ddr3_mem->misc0.active_ranks = 1;
	ddr3_mem->misc0.port_en_0 = 1;
	ddr3_mem->misc0.port_en_1 = 1;
	ddr3_mem->misc0.port_en_2 = 1;

	ddr3_mem->dram_tmg_0.full_reg = 0;
	ddr3_mem->dram_tmg_0.wr2pre = (6+4+9);
	ddr3_mem->dram_tmg_0.t_faw = 27;
	ddr3_mem->dram_tmg_0.t_ras_max = 36;
	ddr3_mem->dram_tmg_0.t_ras_min = 18;
	ddr3_mem->dram_tmg_0.trc = 27;

	ddr3_mem->dram_tmg_1.full_reg = 0;
	ddr3_mem->dram_tmg_1.t_xp = 13;
	ddr3_mem->dram_tmg_1.rd2pre = (0+6);
	ddr3_mem->dram_tmg_1.read_latency = 7;
	ddr3_mem->dram_tmg_1.rd2wr = (7+4+2-6);
	ddr3_mem->dram_tmg_1.wr2rd = (6+4+6);

	ddr3_mem->dram_tmg_2.full_reg = 0;
	ddr3_mem->dram_tmg_2.write_latency = 6;
	ddr3_mem->dram_tmg_2.t_rcd = 7;
	ddr3_mem->dram_tmg_2.t_rrd = 6;
	ddr3_mem->dram_tmg_2.t_ccd = 4;
	ddr3_mem->dram_tmg_2.t_rp = 7;

	ddr3_mem->refr_cnt.full_reg = 0;
	ddr3_mem->refr_cnt.t_rfc_min = 140;

	ddr3_mem->dfi_tmg.full_reg = 0;
	ddr3_mem->dfi_tmg.dfi_t_ctrl_delay = 2;
	ddr3_mem->dfi_tmg.dfi_tphy_wrlat = (6-1);
	ddr3_mem->dfi_tmg.dfi_t_rddata_en = (7-2);
	ddr3_mem->dfi_tmg.dfi_tphy_wrdata = 1;

	ddr3_mem->addrmap0.full_reg = 0;
	ddr3_mem->addrmap0.addrmap_bank_b0 = 7;
	ddr3_mem->addrmap0.addrmap_bank_b1 = 7;
	ddr3_mem->addrmap0.addrmap_bank_b2 = 7;
	ddr3_mem->addrmap0.addrmap_col_b2 = 0;
	ddr3_mem->addrmap0.addrmap_col_b3 = 0;
	ddr3_mem->addrmap0.addrmap_col_b4 = 0;
	ddr3_mem->addrmap0.addrmap_col_b5 = 0;

	ddr3_mem->addrmap1.full_reg = 0;
	ddr3_mem->addrmap1.addrmap_col_b6 = 0;
	ddr3_mem->addrmap1.addrmap_cs_bit0 = 31;
	ddr3_mem->addrmap1.addrmap_col_b7 = 0;
	ddr3_mem->addrmap1.addrmap_col_b8 = 0;
	ddr3_mem->addrmap1.addrmap_col_b9 = 15;
	ddr3_mem->addrmap1.addrmap_col_b10 = 15;
	ddr3_mem->addrmap1.addrmap_col_b11 = 15;

	ddr3_mem->addrmap2.full_reg = 0;
	ddr3_mem->addrmap2.addrmap_row_b0 = 6;
	ddr3_mem->addrmap2.addrmap_row_b1 = 6;
	ddr3_mem->addrmap2.addrmap_row_b2_10 = 6;
	ddr3_mem->addrmap2.addrmap_row_b11 = 6;
	ddr3_mem->addrmap2.addrmap_row_b12 = 6;
	ddr3_mem->addrmap2.addrmap_row_b13 = 6;
	ddr3_mem->addrmap2.addrmap_row_b14 = 6;
	ddr3_mem->addrmap2.addrmap_row_b15 = 15;

	ddr3_mem->misc1.full_reg = 0;

	ddr3_mem->misc2.full_reg = 0;
	ddr3_mem->misc2.dtpr_twtr = 6;
	ddr3_mem->misc2.dtpr_trtp = 6;
	ddr3_mem->misc2.dtpr_trtw = 0;

	ddr3_mem->misc3.full_reg = 0;

	ddr3_mem->misc4.full_reg = 0;
	ddr3_mem->misc4.sel_cpll = CPLL_VALUE;
	ddr3_mem->misc4.ddr_div = 0;
	ddr3_mem->misc4.pgcr_ranken = 1;
	ddr3_mem->misc4.pgcr_rfshdt = 1;
	ddr3_mem->misc4.ddr_remap = 0;

	ddr3_mem->misc5.full_reg = 0;
	ddr3_mem->misc5.dfi_upd_int_min = 12;
	ddr3_mem->misc5.dfi_upd_int_max = 64;

	ddr3_mem->config_0.full_reg = 0;
	ddr3_mem->config_0.phy_mr_cl = 3;
	ddr3_mem->config_0.phy_mr_bl = 0;
	ddr3_mem->config_0.phy_mr_wr = 5;
	ddr3_mem->config_0.phy_mr_cwl = 1;

	ddr3_mem->config_1.full_reg = 0;
	ddr3_mem->config_1.dtpr_tmrd = 0;
	ddr3_mem->config_1.dtpr_tmod = 0;
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
	}
}

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_0_SIZE;
#ifdef CONFIG_SPL_BUILD
	ddr3_t ddr3_mem;
	puts("DDR controllers init started\n");
	set_nt5cb256m8fn_di_cfg(&ddr3_mem);
	uint32_t status = bootrom_DDR_INIT(1, (void*)(&ddr3_mem), (void*)(&ddr3_mem));
	if (status & 0xffff)
		puts("DDR controller #0 init failed\n");
	else
		puts("DDR controller #0 init done\n");

	if ((status >> 16) & 0xffff)
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
