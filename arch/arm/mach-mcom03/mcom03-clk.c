// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2021-2023 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <linux/iopoll.h>

#define SDR_URB_DSP_CTL 0x191004C
#define SDR_URB_DSP_CTL_ENABLE_CLK (BIT(8) | BIT(9))

#define SDR_URB_PCI0_CTL		0x1910050
#define SDR_URB_PCI1_CTL		0x1910054
#define SDR_URB_PCIE_CTL_ENABLE_CLK	BIT(0)
#define SDR_URB_PCIE_CTL_PAD_EN		BIT(4)


int clk_cfg_sdr(void)
{
	// Enable DSP clocks
	writel(SDR_URB_DSP_CTL_ENABLE_CLK, SDR_URB_DSP_CTL);
	// Enable PCIe external clocks
	writel(SDR_URB_PCIE_CTL_ENABLE_CLK | SDR_URB_PCIE_CTL_PAD_EN,
	       SDR_URB_PCI0_CTL);
	writel(SDR_URB_PCIE_CTL_ENABLE_CLK | SDR_URB_PCIE_CTL_PAD_EN,
	       SDR_URB_PCI1_CTL);

	return 0;
}
