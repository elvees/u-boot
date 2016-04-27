/*
 * Copyright (C) 2016 ELVEES NeoTek JSC
 *
 * Copyright (C) 2013 Altera Corporation <www.altera.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/clock_manager.h>

unsigned int cm_get_spi_controller_clk_hz(void)
{
	return SPLL_FREQ >> DIV_SYS1_CTR_VALUE;
}
