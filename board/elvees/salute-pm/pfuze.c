/*
 * Copyright 2017 RnD Center "ELVEES", JSC
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <power/pmic.h>
#include <power/pfuze100_pmic.h>

#define PFUZE100_SWXXMODE_APS		0x08

#ifndef CONFIG_DM_PMIC_PFUZE100
int ddr_poweron(void)
{
	struct pmic *p;
	int ret;

	ret = power_pfuze100_init(PMIC_I2C);
	if (ret)
		return ret;

	p = pmic_get("PFUZE100");
	ret = pmic_probe(p);
	if (ret)
		return ret;

	ret = pmic_reg_write(p, PFUZE100_SW3AMODE, PFUZE100_SWXXMODE_APS);
	ret |= pmic_reg_write(p, PFUZE100_SW3BMODE, PFUZE100_SWXXMODE_APS);

	return ret;
}
#endif
