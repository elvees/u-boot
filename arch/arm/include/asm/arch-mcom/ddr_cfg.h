/*
 * Copyright 2015-2016 ELVEES NeoTek JSC, <www.elvees-nt.com>
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef DDR_CFG_H_
#define DDR_CFG_H_

/* Mainly the following structure fields correspond to DDRMC register
 * fields which is described in 1892VM14YA User Manual in 15.25 chapter.
 * If some fields do not correspond, it's explicitly commented below.
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
			/* Correspond to PHYs PTR0.tdllrst, PTR0.tdlllock,
			 *PTR0.ptr_titmrst, PTR1.tdinit1 fields
			 */
			unsigned ptr_tdllrst:6;
			unsigned ptr_tdlllock:12;
			unsigned ptr_titmrst:4;
			unsigned ptr_tdinit1:8;
		};
	} misc1;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to PHYs PTR1.tdinit0 field */
			unsigned ptr_tdinit0:19;
			/* Correspond to PHYs DCR.ddrmd field */
			unsigned dcr_ddrmd:3;
			/* Correspond to PHYs DTPR0.trtp, DTPR0.twtr,
			 * DTPR0.tccd DTPR1.trtw fields
			 */
			unsigned dtpr_trtp:3;
			unsigned dtpr_twtr:3;
			unsigned dtpr_tccd:1;
			unsigned dtpr_trtw:1;
		};
	} misc2;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to PHYs PTR2.tdinit2 field */
			unsigned ptr_tdinit2:17;
			/* Correspond to PHYs DTPR2.txs field */
			unsigned dtpr_txs:10;
			/* Correspond to PHYs DTPR2.tcke field */
			unsigned dtpr_tcke:4;
		};
	} misc3;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to PHYs DTPR2.tdllk field */
			unsigned tdllk:10;
			/* Correspond to PHYs PGCR.dqscfg field */
			unsigned pgcr_dqscfg:1;
			/* Correspond to PHYs PGCR.dftcmp field */
			unsigned pgcr_dftcmp:1;
			/* Correspond to PHYs PGCR.ranken field */
			unsigned pgcr_ranken:2;
			/* Correspond to PHYs PGCR.rfshdt field */
			unsigned pgcr_rfshdt:2;
			/* Correspond to CMCTRs SEL_CPLL field */
			unsigned sel_cpll:8;
			/* Correspond to CMCTRs DIV_DDR0_CTR, DIV_DDR0_CTR1
			 * fields
			 */
			unsigned ddr_div:2;
			/* Correspond to SMCTRs DDR_REMAP field */
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
			/* Correspond to PHYs MR0.bl field */
			unsigned phy_mr_bl:2;
			/* Correspond to PHYs MR0.cl field */
			unsigned phy_mr_cl:3;
			/* Correspond to PHYs MR0.dr field */
			unsigned phy_mr_dr:1;
			/* Correspond to PHYs MR0.wr field */
			unsigned phy_mr_wr:3;
			/* Correspond to PHYs MR2.cwl field */
			unsigned phy_mr_cwl:3;
			/* Correspond to PHYs MR2.asr field */
			unsigned phy_mr_asr:1;
		};
	} config_0;
	union {
		unsigned int full_reg;
		struct {
			unsigned t_mod:10;
			/* Correspond to PHYs DTPR0.tmrd field */
			unsigned dtpr_tmrd:2;
			/* Correspond to PHYs DTPR0.tmod field */
			unsigned dtpr_tmod:2;
			/* Correspond to PHYs PIR.dramrst field */
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
			/* Correspond to PHYs MR1.bl field */
			unsigned phy_mr_bl:3;
			/* Correspond to PHYs MR1.wc field */
			unsigned phy_mr_wc:1;
			/* Correspond to PHYs MR1.nwr field */
			unsigned phy_mr_nwr:3;
			/* Correspond to PHYs MR2.rl/wl field */
			unsigned phy_mr_rl_wr:4;
			/* Correspond to PHYs MR3.ds field */
			unsigned phy_mr_ds:4;
		};
	} config_0;
	union {
		unsigned int full_reg;
		struct {
			/* Correspond to PHYs DCR.ddr8bnk field */
			unsigned dcr_ddr8bnk:1;
			/* Correspond to PHYs DCR.ddr_type field */
			unsigned dcr_ddr_type:2;
			unsigned t_mrw:10;
			/* Correspond to PHYs PTR2.tdinit3 field */
			unsigned phy_tdinit3:10;
			/* Correspond to PHYs DTPR1.tdqsckmin field */
			unsigned dtpr_tdqsck_min:3;
			/* Correspond to PHYs DTPR1.tdqsckmax field */
			unsigned dtpr_tdqsck_max:3;
		};
	} config_1;

} lpddr2_t;

#endif /* DDR_CFG_H_ */
