/*
 * Copyright 2012-2013 Henrik Nordstrom <henrik@henriknordstrom.net>
 * Copyright 2013 Luke Kenneth Casson Leighton <lkcl@lkcl.net>
 * Copyright 2007-2011 Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * Copyright 2015-2016 ELVEES NeoTek JSC
 * Copyright 2017 RnD Center "ELVEES", OJSC
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#ifdef CONFIG_SPL_BUILD
#include <spl.h>
#include <asm/arch/ddr.h>
#endif
#include <asm/arch/clock.h>
#include <asm/arch/regs.h>
#include <asm/io.h>
#include <linux/kernel.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_SPL_BUILD
/* Set parameters for Micron TwinDie МТ41К512М16-125 SDRAM */
int set_sdram_cfg(struct ddr_cfg *cfg, int tck)
{
	if (!cfg) {
		printf("Invalid pointer to DDR configuration\n");
		return -EINVAL;
	}

	if (!tck) {
		printf("Invalid clock period\n");
		return -EINVAL;
	}

	/* TODO: This function works only for exactly 1066 MHz, because memory
	 * timings could be different for different frequency. */

	cfg->type = MCOM_SDRAM_TYPE_DDR3;

	/* TODO: DDR calibration should be done for IP-KU board.
	 * This ODT/ODS values may not be optimal. */
	cfg->impedance.ods_mc = 40;
	cfg->impedance.ods_dram = 40;
	cfg->impedance.odt_mc = 120;
	cfg->impedance.odt_dram = 120;

	/* TODO: Actually IP-KU SDRAM has 2 ranks. But working with 2 ranks is
	 * untested, so set number of ranks to 1. */
	cfg->common.ranks = MCOM_SDRAM_ONE_RANK;
	cfg->common.banks = 8;
	cfg->common.columns = 1024;
	cfg->common.rows = 32768;
	cfg->common.bl = 8;
	cfg->common.cl = 7;
	cfg->common.cwl = 6;
	cfg->common.twr = max(MCOM_DDR_MIN_TWR, to_clocks(15000, tck));
	cfg->common.tfaw = to_clocks(50000, tck);
	cfg->common.tras = to_clocks(37500, tck);
	cfg->common.tras_max = to_clocks(9 * 7800000, tck);
	cfg->common.trc = to_clocks(50625, tck);
	cfg->common.txp = to_clocks(max(3 * tck, 7500), tck);
	cfg->common.trtp = to_clocks(max(4 * tck, 7500), tck);
	cfg->common.twtr = to_clocks(max(4 * tck, 7500), tck);
	cfg->common.trcd = to_clocks(13125, tck);
	cfg->common.trrd = to_clocks(max(4 * tck, 10000), tck);
	cfg->common.tccd = 4;
	cfg->common.tcke = to_clocks(max(3 * tck, 7500), tck);
	cfg->common.tckesr = cfg->common.tcke + 1;
	cfg->common.tzqcs = 64;
	cfg->common.trefi = to_clocks(7800000, tck);
	cfg->ddr3.trp = to_clocks(13125, tck);
	cfg->ddr3.txpdll = to_clocks(max(10 * tck, 24000), tck);
	cfg->ddr3.tmrd = 4;
	cfg->ddr3.tmod = to_clocks(max(12 * tck, 15000), tck);
	cfg->ddr3.tcksre = to_clocks(max(5 * tck, 10000), tck);
	cfg->ddr3.tcksrx = to_clocks(max(5 * tck, 10000), tck);
	cfg->ddr3.tzqoper = 256;
	cfg->ddr3.trfc = to_clocks(260000, tck);
	cfg->ddr3.tdllk = 512;
	cfg->ddr3.txsdll = cfg->ddr3.tdllk;
	cfg->ddr3.txs = max(5, to_clocks(10000, tck) + (int)cfg->ddr3.trfc);

	return 0;
}
#endif
