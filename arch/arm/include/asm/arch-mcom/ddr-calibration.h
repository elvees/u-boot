/*
 * Copyright 2017 RnD Center "ELVEES", JSC
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _DDR_CALIBRATION_H
#define _DDR_CALIBRATION_H

#include <linux/types.h>
#include <asm/arch/ddr.h>

#define VRAM_BASE	0x3B000000
#define CALIB_MAGIC	0x12345678
#define MAX_CFG_NUM	8194

/**
 * struct ddr_calib_cfg - Configuration used during DDR calibration
 * @impedance:    Impedance parameters for ZQ calibration
 * @dqs_gating:   Array of DQS gating delays. dqs_gating[0] corresponds
 *                lane0, dqs_gating[1] to lane1 and so on.
 */
struct ddr_calib_cfg {
	struct impedance_params impedance;
	u16 dqs_gating[4];
};

/**
 * struct ddr_calib_data - Structure with information for DDR calibration
 * @magic:    Magic value. Calibration will not start if no magic in VRAM.
 * @ctl_id:   DDRMC ID for which calibration should be performed
 * @phase:    Calibration phase
 * @cur_cfg:  Current configuration index in cfg array
 * @prev_cfg: Previous configuration index in cfg array
 * @cfg_num:  Number of checked configurations
 * @status:   Array with DDR stress test status for each configuration
 * @cfg:      Configuration array
 */
struct ddr_calib_data {
	u32 magic;
	u8  ctl_id;
	u8  phase;
	u16 cur_cfg;
	u16 prev_cfg;
	u16 cfg_num;
	struct ddr_calib_cfg cfg[MAX_CFG_NUM];
	u8 status[MAX_CFG_NUM];
};

/**
 * set_calib_cfg - Fixup DDR configuration with DDR calibration parameters
 * @cfgs: Array of DDR configurations
 */
void set_calib_cfg(struct ddr_cfg *cfgs);

#endif /* _DDR_CALIBRATION_H */
