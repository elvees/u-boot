// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 RnD Center "ELVEES", JSC
 *
 * MCom-03 dwc3 glue layer.
 */

#include <clk.h>
#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <linux/bitfield.h>
#include <regmap.h>
#include <reset.h>
#include <syscon.h>

#define USB_PHY_CTR_OFFSET(x)		(0xdc + (x) * 0x34)

#define USB_PHY_CTR_FSEL		GENMASK(29, 24)
#define USB_PHY_CTR_MPLL_MULT		GENMASK(23, 17)
#define USB_PHY_CTR_REF_CLKDIV2		BIT(15)
#define USB_PHY_CTR_REF_SSP_EN		BIT(14)
#define USB_PHY_CTR_REF_USE_PAD		BIT(13)
#define USB_PHY_CTR_SSC_EN		BIT(12)
#define USB_PHY_CTR_SSC_RANGE		GENMASK(11, 9)
#define USB_PHY_CTR_SSC_REF_CLK_SEL	GENMASK(8, 0)

struct dwc3_mcom03 {
	struct clk_bulk		clks;
	struct reset_ctl_bulk	resets;
	struct regmap 		*urb;
	u32			ctrl_id;
};

static int dwc3_mcom03_reset_init(struct udevice *dev,
				  struct dwc3_mcom03 *priv)
{
	int ret;

	ret = reset_get_bulk(dev, &priv->resets);
	if (ret == -ENOTSUPP)
		return 0;
	else if (ret)
		return ret;

	ret = reset_deassert_bulk(&priv->resets);
	if (ret) {
		reset_release_bulk(&priv->resets);
		return ret;
	}

	return 0;
}

static int dwc3_mcom03_clk_init(struct udevice *dev, struct dwc3_mcom03 *priv)
{
	int ret;

	ret = clk_get_bulk(dev, &priv->clks);
	if (ret == -ENOSYS)
		return 0;
	if (ret)
		return ret;

	ret = clk_enable_bulk(&priv->clks);
	if (ret) {
		clk_release_bulk(&priv->clks);
		return ret;
	}

	return 0;
}

static int dwc3_mcom03_phy_init(struct udevice *dev, struct dwc3_mcom03 *priv)
{
	u32 val;
	int ret;
	size_t i;
	ulong ref_clk_rate;
	struct clk ref_clk;

	const struct dwc3_mcom03_phy_cfg {
		u32 refclk_rate;
		u8 fsel, mpll, refclk_div;
		u16 ssc_refclk_sel;
	} phy_cfg[] = {
		/* From Synopsys DesignWare SuperSpeed USB 3.0 femtoPHY
		 * for TSMC 28-nm HPCP 0.9/1.8 V databook, table 3-1 */
		{ 19200000, 0x38, 0, 0, 0 },
		{ 20000000, 0x31, 0, 0, 0 },
		{ 24000000, 0x2a, 0, 0, 0 },
		{ 25000000, 0x2, 0x64, 0, 0 },
		{ 26000000, 0x2, 0x60, 0, 0x108 },
		{ 38400000, 0x38, 0, 1, 0 },
		{ 40000000, 0x31, 0, 1, 0 },
		{ 48000000, 0x2a, 0, 1, 0 },
		{ 50000000, 0x2, 0x64, 1, 0 },
		{ 52000000, 0x2, 0x60, 1, 0x108 },
		{ 100000000, 0x27, 0, 0, 0 },
		{ 200000000, 0x27, 0, 1, 0 },
	};

	ret = clk_get_by_name(dev, "ref", &ref_clk);
	if (ret) {
		dev_err(dev, "'ref' clock not found in DTS\n");
		return ret;
	}

	ref_clk_rate = clk_get_rate(&ref_clk);
	if (ref_clk_rate <= 0) {
		dev_err(dev, "Can't get 'ref' clock rate\n");
		return -ENODEV;
	}

	for (i = 0; i < ARRAY_SIZE(phy_cfg); i++)
		if (ref_clk_rate == phy_cfg[i].refclk_rate)
			break;

	if (i == ARRAY_SIZE(phy_cfg)) {
		dev_err(dev, "Unsupported refclk frequency: %lu\n", ref_clk_rate);
		return -EINVAL;
	}

	val = FIELD_PREP(USB_PHY_CTR_FSEL, phy_cfg[i].fsel) |
	      FIELD_PREP(USB_PHY_CTR_MPLL_MULT, phy_cfg[i].mpll) |
	      FIELD_PREP(USB_PHY_CTR_REF_CLKDIV2, phy_cfg[i].refclk_div) |
	      FIELD_PREP(USB_PHY_CTR_REF_SSP_EN, 1) |
	      FIELD_PREP(USB_PHY_CTR_REF_USE_PAD, 1) |
	      FIELD_PREP(USB_PHY_CTR_SSC_EN, 1) |
	      FIELD_PREP(USB_PHY_CTR_SSC_RANGE, 0) |
	      FIELD_PREP(USB_PHY_CTR_SSC_REF_CLK_SEL,
			 phy_cfg[i].ssc_refclk_sel);

	if (ofnode_read_bool(dev_ofnode(dev), "elvees,clk-ref-alt"))
		val &= ~USB_PHY_CTR_REF_USE_PAD;

	return regmap_write(priv->urb, USB_PHY_CTR_OFFSET(priv->ctrl_id), val);
}

static int dwc3_mcom03_glue_probe(struct udevice *dev)
{
	struct dwc3_mcom03 *priv = dev_get_plat(dev);
	int ret;

	ret = dwc3_mcom03_clk_init(dev, priv);
	if (ret)
		return ret;

	ret = dwc3_mcom03_reset_init(dev, priv);
	if (ret)
		return ret;

	priv->urb = syscon_regmap_lookup_by_phandle(dev, "elvees,urb");
	if (IS_ERR(priv->urb)) {
		dev_err(dev, "Failed to get subsystem URB\n");
		return PTR_ERR(priv->urb);
	}

	ret = ofnode_read_u32(dev_ofnode(dev), "elvees,ctrl-id",
			      &priv->ctrl_id);
	if (ret) {
		dev_err(dev, "Failed to get controller ID\n");
		return ret;
	}

	/* PHY regs should be programmed in reset state */
	ret = reset_assert(&priv->resets.resets[0]);
	if (ret) {
		dev_err(dev, "Failed to assert reset\n");
		return ret;
	}

	/* Wait until PHY goes into reset state, 1 ms should be enough */
	udelay(1000);

	ret = dwc3_mcom03_phy_init(dev, priv);
	if (ret)
		return ret;

	ret = reset_deassert(&priv->resets.resets[0]);
	if (ret) {
		dev_err(dev, "Failed to deassert reset\n");
		return ret;
	}

	/* Wait until PHY goes from reset state, 1 ms should be enough */
	udelay(1000);

	return 0;
}

static int dwc3_mcom03_glue_remove(struct udevice *dev)
{
	struct dwc3_mcom03 *priv = dev_get_plat(dev);

	reset_release_bulk(&priv->resets);

	clk_release_bulk(&priv->clks);

	return dm_scan_fdt_dev(dev);
}

static const struct udevice_id dwc3_mcom03_glue_ids[] = {
	{ .compatible = "elvees,mcom03-dwc3" },
	{ }
};

U_BOOT_DRIVER(dwc3_mcom03) = {
	.name = "dwc3_mcom03_glue",
	.id = UCLASS_SIMPLE_BUS,
	.of_match = dwc3_mcom03_glue_ids,
	.probe = dwc3_mcom03_glue_probe,
	.remove = dwc3_mcom03_glue_remove,
	.plat_auto = sizeof(struct dwc3_mcom03),
	.flags = DM_FLAG_ALLOC_PRIV_DMA,
};
