/*
 * Copyright 2017 RnD Center "ELVEES", JSC
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/regs.h>
#include <asm/arch/ddr-calibration.h>
#include <asm/io.h>

#define DXDLLCR(i, n)	(0x372051CC + (0x2000 * (i)) + (0x40 * (n)))
#define ACDLLCR(i)	(0x37205014 + (0x2000 * (i)))
#define MR1(i)		(0x37205044 + (0x2000 * (i)))
#define ZQ0CR1(i)	(0x37205184 + (0x2000 * (i)))
#define DXCCR(i)	(0x37205028 + (0x2000 * (i)))

#define UNUSED_PARAM	0xff

static inline void write_field(u32 addr, u32 val, u32 mask, u32 offset)
{
	u32 tmp;

	tmp = readl(addr);
	tmp &= ~(mask << offset);
	tmp |= ((val & mask) << offset);
	writel(tmp, addr);
}

static inline u32 read_field(u32 addr, u32 mask, u32 offset)
{
	return (readl(addr) >> offset) & mask;
}

static void enable_clocks(int ctl_id)
{
	cmctr_t *const CMCTR = (cmctr_t *)CMCTR_BASE;

	/* Enable clocks for VRAM */
	CMCTR->GATE_CORE_CTR |= (1 << 5);

	/* Enable clocks for DDR */
	CMCTR->GATE_CORE_CTR |= (1 << (ctl_id + 1));
}

static void print_cfg(int ctl_id)
{
	printf("ZQ0CR1: 0x%02x\n", readl(ZQ0CR1(ctl_id)));
	printf("MR1: 0x%02x\n", readl(MR1(ctl_id)));
	printf("ACMFDLY: 0x%02x\n", read_field(ACDLLCR(ctl_id), 0x3f, 6));

	int i;

	for (i = 0; i < 4; i++)
		printf("LANE %d: SDPHASE 0x%02x, SFDLY 0x%02x, MFDLY 0x%02x\n",
		       i, read_field(DXDLLCR(ctl_id, i), 0xf, 14),
		       read_field(DXDLLCR(ctl_id, i), 0x3f, 0),
		       read_field(DXDLLCR(ctl_id, i), 0x3f, 6));
}

void set_calib_params(int ctl_id)
{
	struct calib_data *data = (struct calib_data *)VRAM_BASE;
	struct calib_cfg *cfg;

	enable_clocks(ctl_id);

	if (data->magic != CALIB_MAGIC) {
		printf("No calibration parameters\n");
		return;
	}

	/* data->cur_cfg_idx should be incremented in Linux.
	 * If it's not occur then we have a hang. */
	if (data->prev_cfg_idx == data->cur_cfg_idx) {
		if (data->cfg_tries >= 3) {
			data->cur_cfg_idx += 1;
			data->cfg_tries = 0;
		} else {
			data->cfg_tries += 1;
		}
	}
	data->prev_cfg_idx = data->cur_cfg_idx;

	if (data->cur_cfg_idx >= data->end_cfg_idx)
		return;

	cfg = &data->cfg[data->cur_cfg_idx];

	if (cfg->lcfg.lane != UNUSED_PARAM) {
		if (cfg->lcfg.sdphase != UNUSED_PARAM) {
			write_field(DXDLLCR(ctl_id, cfg->lcfg.lane),
				    cfg->lcfg.sdphase, 0xf, 14);
		}
		if (cfg->lcfg.sfdly != UNUSED_PARAM) {
			write_field(DXDLLCR(ctl_id, cfg->lcfg.lane),
				    cfg->lcfg.sfdly, 0x3f, 0);
		}
		if (cfg->lcfg.mfdly != UNUSED_PARAM) {
			write_field(DXDLLCR(ctl_id, cfg->lcfg.lane),
				    cfg->lcfg.mfdly, 0x3f, 6);
		}
	}

	if (cfg->acmfdly != UNUSED_PARAM)
		write_field(ACDLLCR(ctl_id), cfg->acmfdly, 0x3f, 6);

	if (cfg->mc_odt_ods != UNUSED_PARAM) {
		if ((cfg->mc_odt_ods >> 4) == 0xf) {
			write_field(ZQ0CR1(ctl_id), cfg->mc_odt_ods, 0xf, 0);
		} else {
			write_field(ZQ0CR1(ctl_id), cfg->mc_odt_ods, 0xff, 0);
			write_field(DXCCR(ctl_id), 1, 1, 0);
		}
	}

	if (cfg->dram_odt_ods != UNUSED_PARAM)
		write_field(MR1(ctl_id), cfg->dram_odt_ods, 0xff, 0);

	print_cfg(ctl_id);
}
