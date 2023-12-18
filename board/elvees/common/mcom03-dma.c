// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021-2023 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <dm.h>
#include <dm/of_access.h>
#include <log.h>

#include <mcom03_sip.h>
#include "mcom03-common.h"

#define mcom03_ddr_subs_sip(id, param) \
	mcom03_sip_smccc_smc(MCOM03_SIP_DDR_SUBS, (id), (param), 0, 0, 0, 0, 0)

int hsperiph_dma32_bus_init(void)
{
	phys_addr_t cpu;
	dma_addr_t bus;
	u64 size;
	ofnode node;
	u32 bar;
	int ret;

	const char *node_name = "/hsperiph@0/dma32-bus";

	node = ofnode_path(node_name);
	if (!ofnode_valid(node)) {
		log_err("Can't find %s DTS node\n", node_name);
		return -ENODATA;
	}

	if (ofnode_device_is_compatible(node, "elvees,mcom03-dma32-hsperiph")) {
		ret = ofnode_get_dma_range(node, &cpu, &bus, &size);
		if (ret) {
			log_err("Can't find 'dma-ranges' property in %s DTS node\n", node_name);
			return ret;
		}

		/* The address in 'dma-ranges' property can be obtained using the equation
		   AxADDR* = {BAR + AxADDR[31:30], AxADDR[29:0]}
		   The setting is required in order to use DDR high address range by the devices.
		 */
		bar = cpu >> 30U;
		ret = mcom03_ddr_subs_sip(MCOM03_SIP_DDR_SUBS_SET_HSPERIPH_BAR, bar);
		if (ret) {
			log_err("Can't set hsperiph bar value 0x%02x\n", bar);
			return ret;
		}
	}

	return 0;
}
