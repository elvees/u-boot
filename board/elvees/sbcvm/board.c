/*
 * (C) Copyright 2012-2013 Henrik Nordstrom <henrik@henriknordstrom.net>
 * (C) Copyright 2013 Luke Kenneth Casson Leighton <lkcl@lkcl.net>
 *
 * (C) Copyright 2007-2011
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 *
 * (C) Copyright 2015
 * ELVEES NeoTek CJSC, <www.elvees-nt.com>
 *
 * Some board init for the Elvees SBC-VM board.
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
	ddr3_mem->dram_tmg_0.wr2pre = (6+4+9); // min value (write_latency + burst_length/2 + tWR) +
	ddr3_mem->dram_tmg_0.t_faw = 27;       // 50 ns (may be 40) +
	ddr3_mem->dram_tmg_0.t_ras_max = 36;   // !
	ddr3_mem->dram_tmg_0.t_ras_min = 18;   // 37.5 ns !!!!!!!!!! must be 20
	ddr3_mem->dram_tmg_0.trc = 27;         // 50.625 ns +

	ddr3_mem->dram_tmg_1.full_reg = 0;
	ddr3_mem->dram_tmg_1.t_xp = 13;        // 24 ns (tXPDLL) +
	ddr3_mem->dram_tmg_1.rd2pre = (0+6);   // tAL + max (tRTP,4) (10 ns) +
	ddr3_mem->dram_tmg_1.read_latency = 7; // 13.1 ns +
	ddr3_mem->dram_tmg_1.rd2wr = (7+4+2-6);// CL + burst_length/2 + 2 - CWL +
	ddr3_mem->dram_tmg_1.wr2rd = (6+4+6);  // CWL + burst_length/2 + tWTR +

	ddr3_mem->dram_tmg_2.full_reg = 0;
	ddr3_mem->dram_tmg_2.write_latency = 6;// CWL +
	ddr3_mem->dram_tmg_2.t_rcd = 7;        // 13.125 ns +
	ddr3_mem->dram_tmg_2.t_rrd = 6;        // 10 ns +
	ddr3_mem->dram_tmg_2.t_ccd = 4;        // 4CK +
	ddr3_mem->dram_tmg_2.t_rp = 7;         // 13.125 ns +

	ddr3_mem->refr_cnt.full_reg = 0;
	ddr3_mem->refr_cnt.t_rfc_min = 140;      // 265 ns +

	ddr3_mem->dfi_tmg.full_reg = 0;
	ddr3_mem->dfi_tmg.dfi_t_ctrl_delay = 2;  // default value (PUBL)
	ddr3_mem->dfi_tmg.dfi_tphy_wrlat = (6-1);    // CWL - 1
	ddr3_mem->dfi_tmg.dfi_t_rddata_en = (7-2);   // CL - 2
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
	ddr3_mem->misc2.dtpr_twtr = 6; // see wr2rd  +
	ddr3_mem->misc2.dtpr_trtp = 6; // see rd2pre +
	ddr3_mem->misc2.dtpr_trtw = 0; // read to write command delay (0 - standart bus turn around) +

	ddr3_mem->misc3.full_reg = 0;

	ddr3_mem->misc4.full_reg = 0;
	ddr3_mem->misc4.sel_cpll = CPLL_VALUE;
	ddr3_mem->misc4.ddr_div = 0;
	ddr3_mem->misc4.pgcr_ranken = 1; // only one rank is enabled for data training
	ddr3_mem->misc4.pgcr_rfshdt = 1;
	ddr3_mem->misc4.ddr_remap = 0;

	ddr3_mem->misc5.full_reg = 0;
	ddr3_mem->misc5.dfi_upd_int_min = 12; //25;  // !
	ddr3_mem->misc5.dfi_upd_int_max = 64; //128; // !

	ddr3_mem->config_0.full_reg = 0;
	ddr3_mem->config_0.phy_mr_cl = 3;
	ddr3_mem->config_0.phy_mr_bl = 0;
	ddr3_mem->config_0.phy_mr_wr = 5; // 9
	ddr3_mem->config_0.phy_mr_cwl = 1;

	ddr3_mem->config_1.full_reg = 0;
	ddr3_mem->config_1.dtpr_tmrd = 0; // this value + 4 +
	ddr3_mem->config_1.dtpr_tmod = 0; // 12CK +
}

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_0_SIZE;
#ifdef CONFIG_SPL_BUILD
	ddr3_t ddr3_mem;
	puts("DDR controller #0 init ... ");
	set_nt5cb256m8fn_di_cfg(&ddr3_mem);
	uint32_t status = bootrom_DDR_INIT(1, (void*)(&ddr3_mem), 0);
	status &= 0xFFFF;
	if (status == 0)
		puts("done\n");
	else
		puts("fail\n");
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
