// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2020 RnD Center "ELVEES", JSC
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
/* Set parameters for ISSI IS43LD32640B-18BLI SDRAM */
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

	cfg->type = MCOM_SDRAM_TYPE_LPDDR2;

	/* This is the best combination of ODS/ODT parameters.
	 * It was found during DDR calibration for board types:
	 * ECAM02DM r1.0.
	 */
	cfg->impedance.ods_mc = 34;
	cfg->impedance.ods_dram = 40;

	cfg->ctl.dqs_gating_override = 0;
	cfg->ctl.dqsres = 5;
	cfg->ctl.dqsnres = 13;

	cfg->common.ranks = MCOM_SDRAM_ONE_RANK;
	cfg->common.banks = 8;
	cfg->common.columns = 512;
	if (IS_ENABLED(CONFIG_TARGET_ECAM02DM3))
		cfg->common.rows = 8192;
	else
		cfg->common.rows = 16384;
	cfg->common.bl = 8;
	cfg->common.cl = 8;
	cfg->common.cwl = 4;
	cfg->common.twr = to_clocks(max(3 * tck, 15000), tck);
	cfg->common.tfaw = to_clocks(max(8 * tck, 50000), tck);
	cfg->common.tras = to_clocks(max(3 * tck, 42000), tck);
	cfg->common.tras_max = to_clocks(70000000, tck);
	cfg->common.trc = to_clocks(max(3 * tck, 42000) +
				    max(3 * tck, 21000), tck);
	cfg->common.txp = to_clocks(max(2 * tck, 7500), tck);
	cfg->common.trtp = to_clocks(max(2 * tck, 7500), tck);
	cfg->common.twtr = to_clocks(max(2 * tck, 7500), tck);
	cfg->common.trcd = to_clocks(max(3 * tck, 18000), tck);
	cfg->common.trrd = to_clocks(max(2 * tck, 10000), tck);
	cfg->common.tccd = 2;
	cfg->common.tcke = 3;
	cfg->common.tckesr = to_clocks(max(3 * tck, 15000), tck);
	cfg->common.tzqcs = to_clocks(max(6 * tck, 90000), tck);
	if (IS_ENABLED(CONFIG_TARGET_ECAM02DM3))
		cfg->common.trefi = to_clocks(7800000, tck);
	else
		cfg->common.trefi = to_clocks(3900000, tck);

	cfg->lpddr2.device_type = MCOM_LPDDR2_TYPE_S4;
	cfg->lpddr2.tdqsck = to_clocks(2500, tck);
	cfg->lpddr2.tdqsck_max = to_clocks(5500, tck);
	if (IS_ENABLED(CONFIG_TARGET_ECAM02DM3))
		cfg->lpddr2.tmrw = 3;
	else
		cfg->lpddr2.tmrw = 5;
	cfg->lpddr2.trpab = to_clocks(max(3 * tck, 21000), tck);
	cfg->lpddr2.trppb = to_clocks(max(3 * tck, 18000), tck);
	cfg->lpddr2.tzqcl = to_clocks(max(6 * tck, 360000), tck);
	cfg->lpddr2.trfcab = to_clocks(130000, tck);
	cfg->lpddr2.txsr = to_clocks(130000 + 10000, tck);
	cfg->lpddr2.tzqreset = to_clocks(max(3 * tck, 50000), tck);

	return 0;
}

int ddr_poweron(void)
{
	static gpio_regs_t *GPIO0 = (gpio_regs_t *)GPIO0_BASE;

	/* GPIOC24 and GPIOC25 are used for DDR controller power enable */
	GPIO0->SWPORTC.CTL &= ~(BIT(25) | BIT(24));
	GPIO0->SWPORTC.DDR |= BIT(25) | BIT(24);
	GPIO0->SWPORTC.DR |= BIT(25) | BIT(24);

	return 0;
}
#endif
