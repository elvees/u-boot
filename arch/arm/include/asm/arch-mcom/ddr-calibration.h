/*
 * Copyright 2017 RnD Center "ELVEES", JSC
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _DDR_CALIBRATION_H
#define _DDR_CALIBRATION_H

#include <linux/types.h>

#define VRAM_BASE	0x3B000000
#define CALIB_MAGIC	0x12345678
#define MAX_CFG_NUM	16384

struct lane_cfg {
	u8 lane;
	u8 sdphase;
	u8 sfdly;
	u8 mfdly;
};

struct calib_cfg {
	u8 mc_odt_ods;
	u8 dram_odt_ods;
	u8 acmfdly;
	u8 is_valid;
	struct lane_cfg lcfg;
};

struct calib_data {
	u32 magic;
	u32 cur_cfg_idx;
	u32 end_cfg_idx;
	u32 prev_cfg_idx;
	u32 cfg_tries;
	struct calib_cfg cfg[MAX_CFG_NUM];
};

void set_calib_params(int ctl_id);

#endif /* _DDR_CALIBRATION_H */
