/*
 * Copyright 2015-2016 ELVEES NeoTek JSC
 * Copyright 2017 RnD Center "ELVEES", OJSC
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <asm/arch/regs.h>

static void disable_ddrmc(int ddrmc_id)
{
	cmctr_t *CMCTR = (cmctr_t *)CMCTR_BASE;

	/* Disable clock for DDRMC */
	if (ddrmc_id)
		CMCTR->GATE_CORE_CTR &= ~4;
	else
		CMCTR->GATE_CORE_CTR &= ~2;

	/* Update DRAM size available for Linux */
	dram_init_banksize();
	printf("DDR controller #%d disabled\n", ddrmc_id);
}

static int do_ddrctl(cmd_tbl_t *cmdtp, int flag, int argc,
		     char * const argv[])
{
	if (argc < 3)
		return cmd_usage(cmdtp);

	int ddrmc_id = (int)simple_strtol(argv[2], NULL, 10);

	if ((ddrmc_id != 0) && (ddrmc_id != 1))
		return cmd_usage(cmdtp);

	if (strcmp(argv[1], "disable") == 0)
		disable_ddrmc(ddrmc_id);
	else
		return cmd_usage(cmdtp);

	return 0;
}

/***************************************************/
#ifndef CONFIG_SPL_BUILD
static char ddrctl_help_text[] =
	"disable 0|1 - Disable DDR controller # 0|1\n";
#endif
U_BOOT_CMD(
	ddrctl,	3,	0,	do_ddrctl,
	"commands to control DDRMC on MCom Platform",
	ddrctl_help_text
);
