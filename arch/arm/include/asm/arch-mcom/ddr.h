/*
 * Copyright 2015-2016 ELVEES NeoTek JSC
 * Copyright 2017 RnD Center "ELVEES", JSC
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _DDR_H
#define _DDR_H

#include <linux/kernel.h>

/* TODO: Add mcom prefix to every public type/macro/function */
enum sdram_ranks {
	MCOM_SDRAM_ONE_RANK = 1,
	MCOM_SDRAM_TWO_RANKS = 3
};

/**
 * struct sdram_params_common - Common SDRAM parameters
 * @ranks:    Number of SDRAM ranks
 * @banks:    Number of SDRAM banks
 * @columns:  Number of SDRAM columns
 * @rows:     Number of SDRAM rows
 * @bl:       Burst length
 * @cl:       CL timing parameter
 * @cwl:      CWL timing parameter
 * @twr:      tWR (min) timing parameter
 * @tfaw:     tFAW (min) timing parameter
 * @tras:     tRAS (min) timing parameter
 * @tras_max: tRAS (max) timing parameter
 * @trc:      tRC (min) timing parameter
 * @txp:      tXP (min) timing parameter
 * @trtp:     tRTP (min) timing parameter
 * @twtr:     tWTR (min) timing parameter
 * @trcd:     tRCD (min) timing parameter
 * @trrd:     tRRD (min) timing parameter
 * @tccd:     tCCD (min) timing parameter
 * @tcke:     tCKE (min) timing parameter
 * @tckesr:   tCKESR (min) timing parameter
 * @tzqcs:    tZQCS (min) timing parameter
 * @trefi:    tREFI timing parameter
 */
struct sdram_params_common {
	enum sdram_ranks ranks;
	u32 banks;
	u32 columns;
	u32 rows;
	u32 bl;
	u32 cl;
	u32 cwl;
	u32 twr;
	u32 tfaw;
	u32 tras;
	u32 tras_max;
	u32 trc;
	u32 txp;
	u32 trtp;
	u32 twtr;
	u32 trcd;
	u32 trrd;
	u32 tccd;
	u32 tcke;
	u32 tckesr;
	u32 tzqcs;
	u32 trefi;
};

/**
 * struct sdram_params_ddr3 - Parameters for DDR3 SDRAMs
 * @trp:     tRP (min) timing parameter
 * @txpdll:  tXPDLL (min) timing parameter
 * @tmrd:    tMRD (min) timing parameter
 * @tmod:    tMOD (min) timing parameter
 * @tcksre:  tCKSRE (min) timing parameter
 * @tcksrx:  tCKSRX (min) timing parameter
 * @tzqoper: tZQoper (min) timing parameter
 * @trfc:    tRFC (min) timing parameter
 * @tdllk:   tDLLK (min) timing parameter
 * @txs:     tXS (min) timing parameter
 * @txsdll:  tXSDLL (min) timing parameter
 */
struct sdram_params_ddr3 {
	u32 trp;
	u32 txpdll;
	u32 tmrd;
	u32 tmod;
	u32 tcksre;
	u32 tcksrx;
	u32 tzqoper;
	u32 trfc;
	u32 tdllk;
	u32 txs;
	u32 txsdll;
};

enum lpddr2_device_type {
	MCOM_LPDDR2_TYPE_S4 = 0,
	MCOM_LPDDR2_TYPE_S2 = 1
};

/**
 * struct sdram_params_lpddr2 - Parameters for LPDDR2 SDRAMs
 * @device_type:     tRP (min) timing parameter
 * @tdqsck:          tDQSCK (min) in ps
 * @tdqsck_max:      tDQSCK (max) in ps
 * @tmrw:            tMRW (min)
 * @trpab:           tRPab (min)
 * @trppb:           tRPpb (min)
 * @tzqcl:           tZQCL (min)
 * @trfcab:          tRFCab (min)
 * @txsr:            tXSR (min)
 */
struct sdram_params_lpddr2 {
	enum lpddr2_device_type device_type;
	u32 tdqsck;
	u32 tdqsck_max;
	u32 tmrw;
	u32 trpab;
	u32 trppb;
	u32 tzqcl;
	u32 trfcab;
	u32 txsr;
	u32 tzqreset;
};

enum sdram_type {
	MCOM_SDRAM_TYPE_DDR3 = 0,
	MCOM_SDRAM_TYPE_LPDDR2 = 1
};

/**
 * struct impedance_params - Impedance parameters for ZQ calibration
 * @ods_mc:       Output driver strength on DDRMC side in Ohms
 * @ods_dram:     Output driver strength on DRAM side in Ohms
 * @odt_mc:       On-die termination on DDRMC side in Ohms (only for DDR3)
 * @odt_dram:     On-die termination on DRAM side in Ohms (only for DDR3)
 */
struct impedance_params {
	u8 ods_mc;
	u8 ods_dram;
	u8 odt_mc;
	u8 odt_dram;
};

/**
 * struct ctl_params - Configuration parameters for DDRMC and PHY
 * @dqsres:       On-die pull-up/down resistor for DQS pin (only for LPDDR2).
 *                dqsres[3] bit selects pull-down (when set to 0) or pull-up
 *                (when set to 1). dqsres[2:0] selects resistor value:
 *                000 - on-die resistor disconnected; 001 - 688 ohms;
 *                010 - 611 ohms; 011 - 550 ohms; 100 - 500 ohms;
 *                101 - 458 ohms; 110 - 393 ohms; 111 - 344 ohms.
 *                The resistor must be used for LPDDR2 to avoid possible
 *                glitches on DQS pin.
 * @dqsnres:      On-die pull-up/down resistor for DQSN pin (only for LPDDR2).
 *                Same encoding as for dqsres.
 */
struct ctl_params {
	u8 dqsres;
	u8 dqsnres;
};

/**
 * struct ddr_cfg - DDR configuration
 * @ctl_id:       DDR Memory Controller ID
 * @type:         SDRAM type (DDR3 or LPDDR2)
 * @common:       Common SDRAM parameters
 * @ddr3/lpddr2:  DDR3/LPDDR2 SDRAM parameters
 */
struct ddr_cfg {
	int ctl_id;
	enum sdram_type type;
	struct impedance_params impedance;
	struct sdram_params_common common;
	struct ctl_params ctl;
	union {
		struct sdram_params_ddr3 ddr3;
		struct sdram_params_lpddr2 lpddr2;
	};
};

enum ddr_return_codes {
	MCOM_DDR_INIT_SUCCESS = 0,
	MCOM_DDR_NO_CFG,
	MCOM_DDR_CFG_ERR,
	MCOM_DDR_TRAIN_ERR,
};

#define to_clocks(t, tck) DIV_ROUND_UP(t, tck)
#define ddr_getrc(status, i) (((status) >> (16 * (i))) & 0xffff)

/**
 * struct ddr_freq - DDR frequency structure
 * @xti_freq:   XTI frequency in HZ
 * @cpll_mult:  CPLL frequency multiplier
 * @ddr0_div:   DDRMC #0 frequency divider
 * @ddr1_div:   DDRMC #1 frequency divider
 */
struct ddr_freq {
	u32 xti_freq;
	u8 cpll_mult;
	u8 ddr0_div;
	u8 ddr1_div;
};

/**
 * ddr_get_clock_period - Get DDR clock period in ps
 * @ctl_id: DDR Memory Controller ID
 * @freq:   DDR frequency structure
 */
u32 ddr_get_clock_period(int ctl_id, struct ddr_freq *freq);

/**
 * mcom_ddr_init - Tries to initialize both DDRMC #0 and DDRMC #1
 * @cfg0: Configuration for DDRMC #0
 * @cfg1: Configuration for DDRMC #1
 * @freq:  DDR frequency structure
 *
 * If @cfgx is NULL appropriate DDRMC will not be initialized.
 *
 * The function returns 0 if both DDRMC has been initialized successfully, or
 * error code otherwise. The lower 16 bit of error code is for DDRMC #0, the
 * upper 16 bit - for DDRMC #1.
 */
u32 mcom_ddr_init(struct ddr_cfg *cfg0, struct ddr_cfg *cfg1,
		  struct ddr_freq *freq);

#define MCOM_DDR_MIN_TWR 5

/**
 * set_sdram_cfg - Fills ddr_cfg structure with values for specific board type
 * @cfg: DDR configuration structure
 * @tck: DDR clock period in ps
 *
 * This function should be implemented for each board type
 */
int set_sdram_cfg(struct ddr_cfg *cfg, int tck);

#endif /* _DDR_H */
