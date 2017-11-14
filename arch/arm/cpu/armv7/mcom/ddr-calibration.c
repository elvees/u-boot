/*
 * Copyright 2017 RnD Center "ELVEES", JSC
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/ddr.h>
#include <asm/arch/ddr-calibration.h>
#include <asm/arch/regs.h>
#include <common.h>

#ifdef CONFIG_DEBUG_DDR_CALIBRATION
static void print_cfg(struct ddr_cfg *cfgs)
{
	int i;

	for (i = 0; i < 2; i++) {
		if (cfgs[i].ctl.dqs_gating_override)
			printf("DQS gating cfg for DDRMC%d: 0x%04x, 0x%04x,"
			       "0x%04x, 0x%04x\n",
			       cfgs[i].ctl_id,
			       cfgs[i].ctl.dqs_gating[0],
			       cfgs[i].ctl.dqs_gating[1],
			       cfgs[i].ctl.dqs_gating[2],
			       cfgs[i].ctl.dqs_gating[3]);

		printf("Impedance cfg for DDRMC%d: 0x%02x, 0x%02x,"
		       "0x%02x, 0x%02x\n",
		      cfgs[i].ctl_id,
		      cfgs[i].impedance.ods_mc,
		      cfgs[i].impedance.ods_dram,
		      cfgs[i].impedance.odt_mc,
		      cfgs[i].impedance.odt_dram);
	}
}
#endif

static void set_cfg(struct ddr_cfg *cfg, struct ddr_calib_cfg *calib_cfg)
{
	int i;

	if (!cfg)
		return;

	memcpy(&cfg->impedance, &calib_cfg->impedance,
	       sizeof(struct impedance_params));

	for (i = 0; i < 4; i++)
		cfg->ctl.dqs_gating[i] = calib_cfg->dqs_gating[i];

	cfg->ctl.dqs_gating_override = 1;
}

void set_calib_cfg(struct ddr_cfg *cfgs)
{
	cmctr_t *const CMCTR = (cmctr_t *)CMCTR_BASE;
	struct ddr_calib_data *data = (struct ddr_calib_data *)VRAM_BASE;
	struct ddr_calib_cfg *calib_cfg;
	int i;

	/* Enable clock for VRAM and check magic. */
	CMCTR->GATE_CORE_CTR |= BIT(5);
	if (data->magic != CALIB_MAGIC) {
		printf("No calibration parameters\n");
		return;
	}

	/* data->cur_cfg should be incremented in Linux. If it's not occur
	 * then we have a hang and should take next cfg. */
	if (data->prev_cfg == data->cur_cfg)
		data->cur_cfg += 1;

	data->prev_cfg = data->cur_cfg;

	if (data->cur_cfg >= data->cfg_num)
		return;

	for (i = 0; i < 2; i++) {
		if (!(data->ctl_id & (1 << i)))
			continue;

		switch (data->phase) {
		case 1:
			calib_cfg = &data->cfg[data->cur_cfg];
			break;
		case 2:
			calib_cfg = &data->cfg[i];
			break;
		default:
			printf("DDR calibration: Unknown phase (%d)\n",
			       data->phase);
			return;
		}

		set_cfg(&cfgs[i], calib_cfg);
	}

	printf("Calibration progress: %d/%d\n", data->cur_cfg, data->cfg_num);

#ifdef CONFIG_DEBUG_DDR_CALIBRATION
	print_cfg(cfgs);
#endif
}
