/*
 * Copyright (C) 2013 Altera Corporation <www.altera.com>
 * Copyright (C) 2016 ELVEES NeoTek JSC
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/arch/clock_manager.h>

int dw_spi_get_clk(struct udevice *bus, ulong *rate)
{
	*rate = SPLL_FREQ >> DIV_SYS1_CTR_VALUE;

	return 0;
}
