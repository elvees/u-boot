// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2026 RnD Center "ELVEES", JSC
 */

#include <dm.h>
#include <dm/device_compat.h>
#include <pci.h>
#include <clk.h>
#include <reset.h>
#include <asm/io.h>
#include <asm/cache.h>
#include <linux/delay.h>
#include <linux/bitfield.h>

#include "pcie_dw_common.h"

#define SYS_CTRL_OFFSET 0x0
#define SYS_CTRL_APP_LTSSM_EN BIT(4)
#define SYS_CTRL_OVRD_LTSSM_EN BIT(31)
#define SYS_CTRL_DEVICE_TYPE_MASK GENMASK(3, 0)

#define DEBUG_ST_OFFSET 0x34
#define DEBUG_ST_RDLH_LINK_UP BIT(0)
#define DEBUG_ST_SMLH_LINK_UP BIT(31)

#define CXPL_DEBUG_INFO1_OFFSET 0x40
#define CXPL_DEBUG_INFO1_SMLH_LTSSM_STATE GENMASK(5, 0)

#define DEVICE_TYPE_EP 0 /* End-Point		Mode */
#define DEVICE_TYPE_RC 4 /* Root Complex		Mode */

#define SYS_JESD_EN_OFFSET 0x300

struct mcom03_pcie {
	struct pcie_dw dw;
	void __iomem *dbi2_base;
	void __iomem *apb_base;
	struct clk_bulk clocks;
	struct reset_ctl_bulk resets;
	u32 num_lanes;
	u32 max_link_speed;
};

static void mcom03_pcie_writel_apb(struct mcom03_pcie *priv, u32 reg, u32 val)
{
	writel(val, priv->apb_base + reg);
}

static void mcom03_pcie_writel_dbi(struct mcom03_pcie *priv, u32 reg, u32 val)
{
	writel(val, priv->dw.dbi_base + reg);
}

static u32 mcom03_pcie_readl_apb(struct mcom03_pcie *priv, u32 reg)
{
	return readl((unsigned char *)priv->apb_base + reg);
}

static u32 mcom03_pcie_readl_dbi(struct mcom03_pcie *priv, u32 reg)
{
	return readl((unsigned char *)priv->dw.dbi_base + reg);
}

static void mcom03_pcie_set_jesd_en_zero(struct mcom03_pcie *priv)
{
	mcom03_pcie_writel_apb(priv, SYS_JESD_EN_OFFSET, 0);
}

static void mcom03_pcie_ltssm_toggle(struct mcom03_pcie *priv, bool val)
{
	u32 reg = mcom03_pcie_readl_apb(priv, SYS_CTRL_OFFSET);

	reg &= ~SYS_CTRL_APP_LTSSM_EN;
	reg |= SYS_CTRL_OVRD_LTSSM_EN | (val ? SYS_CTRL_APP_LTSSM_EN : 0);

	mcom03_pcie_writel_apb(priv, SYS_CTRL_OFFSET, reg);
}

static void mcom03_pcie_set_dev_type(struct mcom03_pcie *priv, u32 device_type)
{
	u32 reg = mcom03_pcie_readl_apb(priv, SYS_CTRL_OFFSET);

	reg &= ~SYS_CTRL_DEVICE_TYPE_MASK;
	reg |= device_type;

	mcom03_pcie_writel_apb(priv, SYS_CTRL_OFFSET, reg);
}

static void mcom03_pcie_configure(struct mcom03_pcie *priv)
{
	u32 val;
	/* Enable write permission for the DBI read-only register */
	dw_pcie_dbi_write_enable(&priv->dw, true);

	/* Disable BAR0 and BAR1 */
	writel(0, priv->dbi2_base + PCI_BASE_ADDRESS_0);
	writel(0, priv->dbi2_base + PCI_BASE_ADDRESS_1);

	/* Configure link speed */
	clrsetbits_le32(priv->dw.dbi_base + PCIE_LINK_CAPABILITY,
			TARGET_LINK_SPEED_MASK, priv->max_link_speed);

	clrsetbits_le32(priv->dw.dbi_base + PCIE_LINK_CTL_2,
			TARGET_LINK_SPEED_MASK, priv->max_link_speed);

	mcom03_pcie_writel_dbi(priv, PCIE_LINK_WIDTH_SPEED_CONTROL,
			       PORT_LOGIC_SPEED_CHANGE);

	/* Set the number of lanes */
	val = mcom03_pcie_readl_dbi(priv, PCIE_PORT_LINK_CONTROL);
	val &= ~PORT_LINK_FAST_LINK_MODE;
	val |= PORT_LINK_DLL_LINK_EN;
	val &= ~PORT_LINK_MODE_MASK;

	switch (priv->num_lanes) {
	case 1:
		val |= PORT_LINK_MODE_1_LANES;
		break;
	case 2:
		val |= PORT_LINK_MODE_2_LANES;
		break;
	case 4:
		val |= PORT_LINK_MODE_4_LANES;
		break;
	default:
		dev_err(priv->dw.dev, "num-lanes %u: invalid value\n",
			priv->num_lanes);
		goto configure_out;
	}
	mcom03_pcie_writel_dbi(priv, PCIE_PORT_LINK_CONTROL, val);

	/* Set link width speed control register */
	val = mcom03_pcie_readl_dbi(priv, PCIE_LINK_WIDTH_SPEED_CONTROL);
	val &= ~PORT_LOGIC_LINK_WIDTH_MASK;
	switch (priv->num_lanes) {
	case 1:
		val |= PORT_LOGIC_LINK_WIDTH_1_LANES;
		break;
	case 2:
		val |= PORT_LOGIC_LINK_WIDTH_2_LANES;
		break;
	case 4:
		val |= PORT_LOGIC_LINK_WIDTH_4_LANES;
		break;
	}
	val |= PORT_LOGIC_SPEED_CHANGE;
	mcom03_pcie_writel_dbi(priv, PCIE_LINK_WIDTH_SPEED_CONTROL, val);

configure_out:
	/* Better disable write permission right after the update */
	dw_pcie_dbi_write_enable(&priv->dw, false);
}

static bool mcom03_pcie_is_link_up(struct mcom03_pcie *priv)
{
	u32 debug_st = mcom03_pcie_readl_apb(priv, DEBUG_ST_OFFSET);

	return (debug_st & DEBUG_ST_SMLH_LINK_UP) &&
	       (debug_st & DEBUG_ST_RDLH_LINK_UP);
}

static int mcom03_pcie_link_up(struct mcom03_pcie *priv)
{
	int retries;

	if (mcom03_pcie_is_link_up(priv)) {
		dev_info(priv->dw.dev,
			 "PCI Link already up before configuration!\n");
		return -EALREADY;
	}

	mcom03_pcie_configure(priv);

	mcom03_pcie_ltssm_toggle(priv, false);
	// TODO: Add PERST# control via GPIO
	mcom03_pcie_ltssm_toggle(priv, true);

	/* Check if the link is up or not */
	for (retries = 0; retries < 50; retries++) {
		if (mcom03_pcie_is_link_up(priv))
			break;
		mdelay(20);
	}

	if (retries >= 10) {
		dev_err(priv->dw.dev, "PCIe-%d Link Fail\n",
			dev_seq(priv->dw.dev));
		return -EIO;
	}

	return 0;
}

static int mcom03_pcie_host_init(struct mcom03_pcie *priv)
{
	struct udevice *dev = priv->dw.dev;
	struct udevice *ctrl = pci_get_controller(dev);
	struct pci_controller *hose = dev_get_uclass_priv(ctrl);
	int ret;

	ret = clk_enable_bulk(&priv->clocks);
	if (ret) {
		dev_err(dev, "failed to enable clocks: (%d)\n", ret);
		return ret;
	}

	ret = reset_deassert_bulk(&priv->resets);
	if (ret) {
		dev_err(dev, "failed to deassert resets: (%d)\n", ret);
		goto err_reset;
	}

	mcom03_pcie_set_dev_type(priv, DEVICE_TYPE_RC);
	mcom03_pcie_set_jesd_en_zero(priv);

	pcie_dw_setup_host(&priv->dw);

	ret = mcom03_pcie_link_up(priv);
	if (ret && ret != -EALREADY) {
		dev_info(dev, "PCIe-%d: Link Down (Gen%d-x%d, Bus%d)\n",
			 dev_seq(dev), pcie_dw_get_link_speed(&priv->dw),
			 pcie_dw_get_link_width(&priv->dw), hose->first_busno);
		goto err_link_up;
	}

	dev_info(dev, "PCIe-%d: Link Up (Gen%d-x%d, Bus%d)\n", dev_seq(dev),
		 pcie_dw_get_link_speed(&priv->dw),
		 pcie_dw_get_link_width(&priv->dw), hose->first_busno);

	return 0;

err_link_up:
	reset_assert_bulk(&priv->resets);
err_reset:
	clk_disable_bulk(&priv->clocks);
	return ret;
}

static int mcom03_pcie_read_addr(struct udevice *dev, const char *name,
				 void **ptr)
{
	*ptr = dev_read_addr_name_ptr(dev, name);

	if (!(*ptr)) {
		dev_err(dev, "No %s address!\n", name);
		return -EINVAL;
	}

	dev_dbg(dev, "%s address: 0x%lx\n", name, (uintptr_t)*ptr);

	return 0;
};

static int mcom03_pcie_read_addr_size(struct udevice *dev, const char *name,
				      void **ptr, fdt_size_t *size)
{
	*ptr = dev_read_addr_size_name_ptr(dev, name, size);

	if (!(*ptr)) {
		dev_err(dev, "No %s address!\n", name);
		return -EINVAL;
	}
	dev_dbg(dev, "%s address: 0x%lx, size: 0x%llx\n", name, (uintptr_t)*ptr,
		*size);

	return 0;
}

static int mcom03_pcie_of_to_plat(struct udevice *dev)
{
	struct mcom03_pcie *priv = dev_get_priv(dev);
	int ret = 0;

	ret = mcom03_pcie_read_addr(dev, "dbi", &priv->dw.dbi_base);
	if (ret)
		goto err_of_to_plat;

	ret = mcom03_pcie_read_addr(dev, "dbi2", &priv->dbi2_base);
	if (ret)
		goto err_of_to_plat;

	ret = mcom03_pcie_read_addr(dev, "atu", &priv->dw.atu_base);
	if (ret)
		goto err_of_to_plat;

	ret = mcom03_pcie_read_addr(dev, "apb", &priv->apb_base);
	if (ret)
		goto err_of_to_plat;

	ret = mcom03_pcie_read_addr_size(dev, "config", &priv->dw.cfg_base,
					 &priv->dw.cfg_size);
	if (ret)
		goto err_of_to_plat;

	ret = dev_read_u32(dev, "num-lanes", &priv->num_lanes);
	if (ret)
		goto err_of_to_plat;

	priv->max_link_speed =
		dev_read_u32_default(dev, "max-link-speed", LINK_SPEED_GEN_3);

	ret = clk_get_bulk(dev, &priv->clocks);
	if (ret) {
		dev_err(dev, "failed to get clocks: (%d)\n", ret);
		goto err_of_to_plat;
	}

	ret = reset_get_bulk(dev, &priv->resets);
	if (ret) {
		dev_err(dev, "failed to get resets: (%d)\n", ret);
		goto err_reset;
	}

	return 0;

err_reset:
	clk_release_bulk(&priv->clocks);
err_of_to_plat:
	return ret;
}

static int mcom03_pcie_probe(struct udevice *dev)
{
	struct mcom03_pcie *priv = dev_get_priv(dev);
	int ret = 0;

	priv->dw.first_busno = dev_seq(dev);
	priv->dw.dev = dev;

	ret = mcom03_pcie_host_init(priv);
	if (ret) {
		dev_err(dev, "failed to init host: (%d)\n", ret);
		return ret;
	}

	ret = pcie_dw_prog_outbound_atu_unroll(&priv->dw, PCIE_ATU_REGION_INDEX0,
					       PCIE_ATU_TYPE_MEM,
					       priv->dw.mem.phys_start,
					       priv->dw.mem.bus_start, priv->dw.mem.size);
	return ret;
}

static int mcom03_pcie_remove(struct udevice *dev)
{
	struct mcom03_pcie *priv = dev_get_priv(dev);

	mcom03_pcie_ltssm_toggle(priv, false);
	reset_assert_bulk(&priv->resets);
	reset_release_bulk(&priv->resets);
	clk_release_bulk(&priv->clocks);

	return 0;
}

static const struct dm_pci_ops mcom03_pcie_ops = {
	.read_config = pcie_dw_read_config,
	.write_config = pcie_dw_write_config,
};

static const struct udevice_id mcom03_pcie_ids[] = {
	{ .compatible = "elvees,mcom03-pcie" },
	{ /* sentinel */ },
};

U_BOOT_DRIVER(mcom03_pcie) = {
	.name = "mcom03-pcie",
	.id = UCLASS_PCI,
	.of_match = mcom03_pcie_ids,
	.ops = &mcom03_pcie_ops,
	.probe = mcom03_pcie_probe,
	.remove = mcom03_pcie_remove,
	.of_to_plat = mcom03_pcie_of_to_plat,
	.priv_auto = sizeof(struct mcom03_pcie),
};
