/*
 * Copyright 2015-2016 ELVEES NeoTek JSC, <www.elvees-nt.com>
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef DDR_CFG_H_
#define DDR_CFG_H_

/* Mainly the following structure fields correspond to DDRMC register
 * fields which is described in 1892VM14YA User Manual in 15.25 chapter.
 */
typedef struct {
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to DDRMC DRAMTMG0.t_ras_min */
			unsigned t_ras_min:6;
			/* Correspond to DDRMC DRAMTMG0.t_ras_max */
			unsigned t_ras_max:7;
			/* Correspond to DDRMC DRAMTMG0.t_faw */
			unsigned t_faw:6;
			/* Correspond to DDRMC DRAMTMG0.wr2pre */
			unsigned wr2pre:6;
			/* Correspond to DDRMC DRAMTMG1.t_rc */
			unsigned trc:6;
		};
	} dram_tmg_0;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to DDRMC DRAMTMG1.rd2pre */
			unsigned rd2pre:5;
			/* Correspond to DDRMC DRAMTMG1.t_xp */
			unsigned t_xp:5;
			/* Correspond to DDRMC DRAMTMG2.wr2rd */
			unsigned wr2rd:6;
			/* Correspond to DDRMC DRAMTMG2.rd2wr */
			unsigned rd2wr:5;
			/* Correspond to DDRMC DRAMTMG2.read_latency */
			unsigned read_latency:6;
		};
	} dram_tmg_1;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to DDRMC DRAMTMG2.write_latency */
			unsigned write_latency:6;
			/* Correspond to DDRMC DRAMTMG3.t_mrd */
			unsigned t_mrd:6;
			/* Correspond to DDRMC DRAMTMG4.t_rp */
			unsigned t_rp:5;
			/* Correspond to DDRMC DRAMTMG4.t_rrd */
			unsigned t_rrd:4;
			/* Correspond to DDRMC DRAMTMG4.t_ccd */
			unsigned t_ccd:3;
			/* Correspond to DDRMC DRAMTMG4.t_rcd */
			unsigned t_rcd:5;
		};
	} dram_tmg_2;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to DDRMC DRAMTMG5.t_cke */
			unsigned t_cke:4;
			/* Correspond to DDRMC DRAMTMG5.t_ckesr */
			unsigned t_ckesr:6;
			/* Correspond to DDRMC DRAMTMG5.t_cksre */
			unsigned t_cksre:4;
			/* Correspond to DDRMC DRAMTMG5.t_cksrx */
			unsigned t_cksrx:4;
			/* Correspond to DDRMC DRAMTMG8.post_selfref_gap_x32 */
			unsigned post_selfref_gap:7;
		};
	} dram_tmg_3;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to DDRMC MSTR.active_ranks */
			unsigned active_ranks:2;
			/* Correspond to DDRMC MSTR.burst_rdwr */
			unsigned burst_rdwr:4;
			/* Correspond to DDRMC MSTR.ddr3 */
			unsigned ddr3_type:1;
			/* Correspond to DDRMC MSTR.lpddr2 */
			unsigned lpddr2_type:1;
			/* Correspond to DDRMC PWRTMG.selfref_to_x32 */
			unsigned selfref_to_x32:8;
			/* Correspond to DDRMC PCTRL_0.port_en */
			unsigned port_en_0:1;
			/* Correspond to DDRMC PCTRL_1.port_en */
			unsigned port_en_1:1;
			/* Correspond to DDRMC PCTRL_2.port_en */
			unsigned port_en_2:1;
		};
	} misc0;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to DDRMC RFSHCTL0.refresh_margin */
			unsigned refresh_margin:4;
			/* Correspond to DDRMC RFSHCTL0.refresh_to_x32 */
			unsigned refresh_to_x32:5;
			/* Correspond to DDRMC RFSHCTL0.refresh_burst */
			unsigned refresh_burst:3;
			/* Correspond to DDRMC RFSHCTL0.per_bank_refresh */
			unsigned per_bank_refresh:1;
			/* Correspond to DDRMC RFSHTMG.t_rfc_nom_x32 */
			unsigned t_rfc_nom_x32:8;
			/* Correspond to DDRMC RFSHTMG.t_rfc_min */
			unsigned t_rfc_min:9;
		};
	} refr_cnt;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to DDRMC DFITMG0.dfi_t_ctrl_delay */
			unsigned dfi_t_ctrl_delay:5;
			/* Correspond to DDRMC DFITMG0.dfi_t_rddata_en */
			unsigned dfi_t_rddata_en:6;
			/* Correspond to DDRMC DFITMG0.dfi_tphy_wrdata */
			unsigned dfi_tphy_wrdata:6;
			/* Correspond to DDRMC DFITMG0.dfi_tphy_wrlat */
			unsigned dfi_tphy_wrlat:6;
			/* Correspond to DDRMC
			 * DFITMG1.dfi_t_dram_clk_disable
			 */
			unsigned dfi_t_dram_clk_disable:4;
			/* Correspond to DDRMC DFITMG1.dfi_t_dram_clk_enable */
			unsigned dfi_t_dram_clk_enable:4;
		};
	} dfi_tmg;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to DDRMC ADDRMAP1.addrmap_bank_b0 */
			unsigned addrmap_bank_b0:4;
			/* Correspond to DDRMC ADDRMAP1.addrmap_bank_b1 */
			unsigned addrmap_bank_b1:4;
			/* Correspond to DDRMC ADDRMAP1.addrmap_bank_b2 */
			unsigned addrmap_bank_b2:4;
			/* Correspond to DDRMC ADDRMAP2.addrmap_col_b2 */
			unsigned addrmap_col_b2:4;
			/* Correspond to DDRMC ADDRMAP2.addrmap_col_b3 */
			unsigned addrmap_col_b3:4;
			/* Correspond to DDRMC ADDRMAP2.addrmap_col_b4 */
			unsigned addrmap_col_b4:4;
			/* Correspond to DDRMC ADDRMAP2.addrmap_col_b5 */
			unsigned addrmap_col_b5:4;
		};
	} addrmap0;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to DDRMC ADDRMAP0.addrmap_cs_bit0 */
			unsigned addrmap_cs_bit0:5;
			/* Correspond to DDRMC ADDRMAP3.addrmap_col_b6 */
			unsigned addrmap_col_b6:4;
			/* Correspond to DDRMC ADDRMAP3.addrmap_col_b7 */
			unsigned addrmap_col_b7:4;
			/* Correspond to DDRMC ADDRMAP3.addrmap_col_b8 */
			unsigned addrmap_col_b8:4;
			/* Correspond to DDRMC ADDRMAP3.addrmap_col_b9 */
			unsigned addrmap_col_b9:4;
			/* Correspond to DDRMC ADDRMAP4.addrmap_col_b10 */
			unsigned addrmap_col_b10:4;
			/* Correspond to DDRMC ADDRMAP4.addrmap_col_b11 */
			unsigned addrmap_col_b11:4;
		};
	} addrmap1;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to DDRMC ADDRMAP5.addrmap_row_b0 */
			unsigned addrmap_row_b0:4;
			/* Correspond to DDRMC ADDRMAP5.addrmap_row_b1 */
			unsigned addrmap_row_b1:4;
			/* Correspond to DDRMC ADDRMAP5.addrmap_row_b2_10 */
			unsigned addrmap_row_b2_10:4;
			/* Correspond to DDRMC ADDRMAP5.addrmap_row_b11 */
			unsigned addrmap_row_b11:4;
			/* Correspond to DDRMC ADDRMAP6.addrmap_row_b12 */
			unsigned addrmap_row_b12:4;
			/* Correspond to DDRMC ADDRMAP6.addrmap_row_b13 */
			unsigned addrmap_row_b13:4;
			/* Correspond to DDRMC ADDRMAP6.addrmap_row_b14 */
			unsigned addrmap_row_b14:4;
			/* Correspond to DDRMC ADDRMAP6.addrmap_row_b15 */
			unsigned addrmap_row_b15:4;
		};
	} addrmap2;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to PHY PTR0.tdllrst */
			unsigned ptr_tdllrst:6;
			/* Correspond to PHY PTR0.tdlllock */
			unsigned ptr_tdlllock:12;
			/* Correspond to PHY PTR0.ptr_titmrst */
			unsigned ptr_titmrst:4;
			/* Correspond to PHY PTR1.tdinit1 */
			unsigned ptr_tdinit1:8;
		};
	} misc1;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to PHY PTR1.tdinit0 */
			unsigned ptr_tdinit0:19;
			/* Correspond to PHY DCR.ddrmd */
			unsigned dcr_ddrmd:3;
			/* Correspond to PHY DTPR0.trtp */
			unsigned dtpr_trtp:3;
			/* Correspond to PHY DTPR0.twtr */
			unsigned dtpr_twtr:3;
			/* Correspond to PHY DTPR0.tccd */
			unsigned dtpr_tccd:1;
			/* Correspond to PHY DTPR1.trtw */
			unsigned dtpr_trtw:1;
		};
	} misc2;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to PHY PTR2.tdinit2 */
			unsigned ptr_tdinit2:17;
			/* Correspond to PHY DTPR2.txs */
			unsigned dtpr_txs:10;
			/* Correspond to PHY DTPR2.tcke */
			unsigned dtpr_tcke:4;
		};
	} misc3;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to PHY DTPR2.tdllk */
			unsigned tdllk:10;
			/* Correspond to PHY PGCR.dqscfg */
			unsigned pgcr_dqscfg:1;
			/* Correspond to PHY PGCR.dftcmp */
			unsigned pgcr_dftcmp:1;
			/* Correspond to PHY PGCR.ranken */
			unsigned pgcr_ranken:2;
			/* Correspond to PHY PGCR.rfshdt */
			unsigned pgcr_rfshdt:2;
			/* Correspond to CMCTR SEL_CPLL */
			unsigned sel_cpll:8;
			/* Correspond to CMCTR DIV_DDR0_CTR, DIV_DDR0_CTR1 */
			unsigned ddr_div:2;
			/* Correspond to SMCTR DDR_REMAP */
			unsigned ddr_remap:1;
		};
	} misc4;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to DDRMC
			 * DFIUPD1.dfi_t_ctrlupd_interval_min_x1024
			 */
			unsigned dfi_upd_int_min:8;
			/* Correspond to DDRMC
			 * DFIUPD1.dfi_t_ctrlupd_interval_max_x1024
			 */
			unsigned dfi_upd_int_max:8;
		};
	} misc5;

	/* Used to save some PHY register values which is got during DRAM
	 * trainings
	 */
	unsigned int dump[32];
} ddr_common_t;

/* The ddr3_t structure is the same ddr_common_t structure but with 2
 * additional unions, specific for ddr3: config_0, config_1.
 */
typedef struct {
	union {
		unsigned int full_reg;
		struct {
			unsigned t_ras_min:6;
			unsigned t_ras_max:7;
			unsigned t_faw:6;
			unsigned wr2pre:6;
			unsigned trc:6;
		};
	} dram_tmg_0;
	union {
		unsigned int full_reg;
		struct {
			unsigned rd2pre:5;
			unsigned t_xp:5;
			unsigned wr2rd:6;
			unsigned rd2wr:5;
			unsigned read_latency:6;
		};
	} dram_tmg_1;
	union {
		unsigned int full_reg;
		struct {
			unsigned write_latency:6;
			unsigned t_mrd:6;
			unsigned t_rp:5;
			unsigned t_rrd:4;
			unsigned t_ccd:3;
			unsigned t_rcd:5;
		};
	} dram_tmg_2;
	union {
		unsigned int full_reg;
		struct {
			unsigned t_cke:4;
			unsigned t_ckesr:6;
			unsigned t_cksre:4;
			unsigned t_cksrx:4;
			unsigned post_selfref_gap:7;
		};
	} dram_tmg_3;
	union {
		unsigned int full_reg;
		struct {
			unsigned active_ranks:2;
			unsigned burst_rdwr:4;
			unsigned ddr3_type:1;
			unsigned lpddr2_type:1;
			unsigned selfref_to_x32:8;
			unsigned port_en_0:1;
			unsigned port_en_1:1;
			unsigned port_en_2:1;
		};
	} misc0;
	union {
		unsigned int full_reg;
		struct {
			unsigned refresh_margin:4;
			unsigned refresh_to_x32:5;
			unsigned refresh_burst:3;
			unsigned per_bank_refresh:1;
			unsigned t_rfc_nom_x32:8;
			unsigned t_rfc_min:9;
		};
	} refr_cnt;
	union {
		unsigned int full_reg;
		struct {
			unsigned dfi_t_ctrl_delay:5;
			unsigned dfi_t_rddata_en:6;
			unsigned dfi_tphy_wrdata:6;
			unsigned dfi_tphy_wrlat:6;
			unsigned dfi_t_dram_clk_disable:4;
			unsigned dfi_t_dram_clk_enable:4;
		};
	} dfi_tmg;
	union {
		unsigned int full_reg;
		struct {
			unsigned addrmap_bank_b0:4;
			unsigned addrmap_bank_b1:4;
			unsigned addrmap_bank_b2:4;
			unsigned addrmap_col_b2:4;
			unsigned addrmap_col_b3:4;
			unsigned addrmap_col_b4:4;
			unsigned addrmap_col_b5:4;
		};
	} addrmap0;
	union {
		unsigned int full_reg;
		struct {
			unsigned addrmap_cs_bit0:5;
			unsigned addrmap_col_b6:4;
			unsigned addrmap_col_b7:4;
			unsigned addrmap_col_b8:4;
			unsigned addrmap_col_b9:4;
			unsigned addrmap_col_b10:4;
			unsigned addrmap_col_b11:4;
		};
	} addrmap1;
	union {
		unsigned int full_reg;
		struct {
			unsigned addrmap_row_b0:4;
			unsigned addrmap_row_b1:4;
			unsigned addrmap_row_b2_10:4;
			unsigned addrmap_row_b11:4;
			unsigned addrmap_row_b12:4;
			unsigned addrmap_row_b13:4;
			unsigned addrmap_row_b14:4;
			unsigned addrmap_row_b15:4;
		};
	} addrmap2;
	union {
		unsigned int full_reg;
		struct {
			unsigned ptr_tdllrst:6;
			unsigned ptr_tdlllock:12;
			unsigned ptr_titmrst:4;
			unsigned ptr_tdinit1:8;
		};
	} misc1;
	union {
		unsigned int full_reg;
		struct {
			unsigned ptr_tdinit0:19;
			unsigned dcr_ddrmd:3;
			unsigned dtpr_trtp:3;
			unsigned dtpr_twtr:3;
			unsigned dtpr_tccd:1;
			unsigned dtpr_trtw:1;
		};
	} misc2;
	union {
		unsigned int full_reg;
		struct {
			unsigned ptr_tdinit2:17;
			unsigned dtpr_txs:10;
			unsigned dtpr_tcke:4;
		};
	} misc3;
	union {
		unsigned int full_reg;
		struct {
			unsigned tdllk:10;
			unsigned pgcr_dqscfg:1;
			unsigned pgcr_dftcmp:1;
			unsigned pgcr_ranken:2;
			unsigned pgcr_rfshdt:2;
			unsigned sel_cpll:8;
			unsigned ddr_div:2;
			unsigned ddr_remap:1;
		};
	} misc4;
	union {
		unsigned int full_reg;
		struct {
			unsigned dfi_upd_int_min:8;
			unsigned dfi_upd_int_max:8;
		};
	} misc5;

	unsigned int dump[32];

	union {
		unsigned int full_reg;
		struct {
			/* Correspond to PHY MR0.bl */
			unsigned phy_mr_bl:2;
			/* Correspond to PHY MR0.cl */
			unsigned phy_mr_cl:3;
			/* Correspond to PHY MR0.dr */
			unsigned phy_mr_dr:1;
			/* Correspond to PHY MR0.wr */
			unsigned phy_mr_wr:3;
			/* Correspond to PHY MR2.cwl */
			unsigned phy_mr_cwl:3;
			/* Correspond to PHY MR2.asr */
			unsigned phy_mr_asr:1;
		};
	} config_0;
	union {
		unsigned int full_reg;
		struct {
			unsigned t_mod:10;
			/* Correspond to PHY DTPR0.tmrd */
			unsigned dtpr_tmrd:2;
			/* Correspond to PHY DTPR0.tmod */
			unsigned dtpr_tmod:2;
			/* Correspond to PHY PIR.dramrst */
			unsigned pir_dram_rst:1;
		};
	} config_1;

} ddr3_t;

/* The lpddr2_t structure is the same ddr_common_t structure but with 2
 * additional unions, specific for lpddr2: config_0, config_1.
 */
typedef struct {
	union {
		unsigned int full_reg;
		struct {
			unsigned t_ras_min:6;
			unsigned t_ras_max:7;
			unsigned t_faw:6;
			unsigned wr2pre:6;
			unsigned trc:6;
		};
	} dram_tmg_0;
	union {
		unsigned int full_reg;
		struct {
			unsigned rd2pre:5;
			unsigned t_xp:5;
			unsigned wr2rd:6;
			unsigned rd2wr:5;
			unsigned read_latency:6;
		};
	} dram_tmg_1;
	union {
		unsigned int full_reg;
		struct {
			unsigned write_latency:6;
			unsigned t_mrd:6;
			unsigned t_rp:5;
			unsigned t_rrd:4;
			unsigned t_ccd:3;
			unsigned t_rcd:5;
		};
	} dram_tmg_2;
	union {
		unsigned int full_reg;
		struct {
			unsigned t_cke:4;
			unsigned t_ckesr:6;
			unsigned t_cksre:4;
			unsigned t_cksrx:4;
			unsigned post_selfref_gap:7;
		};
	} dram_tmg_3;
	union {
		unsigned int full_reg;
		struct {
			unsigned active_ranks:2;
			unsigned burst_rdwr:4;
			unsigned ddr3_type:1;
			unsigned lpddr2_type:1;
			unsigned selfref_to_x32:8;
			unsigned port_en_0:1;
			unsigned port_en_1:1;
			unsigned port_en_2:1;
		};
	} misc0;
	union {
		unsigned int full_reg;
		struct {
			unsigned refresh_margin:4;
			unsigned refresh_to_x32:5;
			unsigned refresh_burst:3;
			unsigned per_bank_refresh:1;
			unsigned t_rfc_nom_x32:8;
			unsigned t_rfc_min:9;
		};
	} refr_cnt;
	union {
		unsigned int full_reg;
		struct {
			unsigned dfi_t_ctrl_delay:5;
			unsigned dfi_t_rddata_en:6;
			unsigned dfi_tphy_wrdata:6;
			unsigned dfi_tphy_wrlat:6;
			unsigned dfi_t_dram_clk_disable:4;
			unsigned dfi_t_dram_clk_enable:4;
		};
	} dfi_tmg;
	union {
		unsigned int full_reg;
		struct {
			unsigned addrmap_bank_b0:4;
			unsigned addrmap_bank_b1:4;
			unsigned addrmap_bank_b2:4;
			unsigned addrmap_col_b2:4;
			unsigned addrmap_col_b3:4;
			unsigned addrmap_col_b4:4;
			unsigned addrmap_col_b5:4;
		};
	} addrmap0;
	union {
		unsigned int full_reg;
		struct {
			unsigned addrmap_cs_bit0:5;
			unsigned addrmap_col_b6:4;
			unsigned addrmap_col_b7:4;
			unsigned addrmap_col_b8:4;
			unsigned addrmap_col_b9:4;
			unsigned addrmap_col_b10:4;
			unsigned addrmap_col_b11:4;
		};
	} addrmap1;
	union {
		unsigned int full_reg;
		struct {
			unsigned addrmap_row_b0:4;
			unsigned addrmap_row_b1:4;
			unsigned addrmap_row_b2_10:4;
			unsigned addrmap_row_b11:4;
			unsigned addrmap_row_b12:4;
			unsigned addrmap_row_b13:4;
			unsigned addrmap_row_b14:4;
			unsigned addrmap_row_b15:4;
		};
	} addrmap2;
	union {
		unsigned int full_reg;
		struct {
			unsigned ptr_tdllrst:6;
			unsigned ptr_tdlllock:12;
			unsigned ptr_titmrst:4;
			unsigned ptr_tdinit1:8;
		};
	} misc1;
	union {
		unsigned int full_reg;
		struct {
			unsigned ptr_tdinit0:19;
			unsigned dcr_ddrmd:3;
			unsigned dtpr_trtp:3;
			unsigned dtpr_twtr:3;
			unsigned dtpr_tccd:1;
			unsigned dtpr_trtw:1;
		};
	} misc2;
	union {
		unsigned int full_reg;
		struct {
			unsigned ptr_tdinit2:17;
			unsigned dtpr_txs:10;
			unsigned dtpr_tcke:4;
		};
	} misc3;
	union {
		unsigned int full_reg;
		struct {
			unsigned tdllk:10;
			unsigned pgcr_dqscfg:1;
			unsigned pgcr_dftcmp:1;
			unsigned pgcr_ranken:2;
			unsigned pgcr_rfshdt:2;
			unsigned sel_cpll:8;
			unsigned ddr_div:2;
			unsigned ddr_remap:1;
		};
	} misc4;
	union {
		unsigned int full_reg;
		struct {
			unsigned dfi_upd_int_min:8;
			unsigned dfi_upd_int_max:8;
		};
	} misc5;

	unsigned int dump[32];

	union {
		unsigned int full_reg;
		struct {
			/* Correspond to PHY MR1.bl */
			unsigned phy_mr_bl:3;
			/* Correspond to PHY MR1.wc */
			unsigned phy_mr_wc:1;
			/* Correspond to PHY MR1.nwr */
			unsigned phy_mr_nwr:3;
			/* Correspond to PHY MR2.rl/wl */
			unsigned phy_mr_rl_wr:4;
			/* Correspond to PHY MR3.ds */
			unsigned phy_mr_ds:4;
		};
	} config_0;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to PHY DCR.ddr8bnk */
			unsigned dcr_ddr8bnk:1;
			/* Correspond to PHY DCR.ddr_type */
			unsigned dcr_ddr_type:2;
			unsigned t_mrw:10;
			/* Correspond to PHY PTR2.tdinit3 */
			unsigned phy_tdinit3:10;
			/* Correspond to PHY DTPR1.tdqsckmin */
			unsigned dtpr_tdqsck_min:3;
			/* Correspond to PHY DTPR1.tdqsckmax */
			unsigned dtpr_tdqsck_max:3;
		};
	} config_1;

} lpddr2_t;

#endif /* DDR_CFG_H_ */
