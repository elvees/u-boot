// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2020-2026 RnD Center "ELVEES", JSC
 */

#include <asm/io.h>
#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <reset-uclass.h>
#include <syscon.h>
#include <regmap.h>
#include <dt-bindings/soc/elvees,mcom03.h>

#define WRITE_ENABLE_OFFSET	16

#define PP_PCIE_BTN	BIT(1)

#define RST_MONO_ON		BIT(0)
#define SDR_PCI0_PERSTN_MODE	BIT(0)
#define SDR_PCI0_PERSTN		BIT(1)
#define SDR_PCI1_PERSTN_MODE	BIT(8)
#define SDR_PCI1_PERSTN		BIT(9)

enum rst_reg_type {
	RST_MONO,
	RST_PCIE_BTN,
	RST_PCIE_PERSTN_PAD,
};

struct sdr_reset {
	u32 id;
	u32 offset;
	enum rst_reg_type type;
};

static const struct sdr_reset sdr_reset_map[] = {
	{
		.id	= SDR_RST_PCI0_BTN,
		.offset	= 0x0,
		.type	= RST_PCIE_BTN,
	},
	{
		.id	= SDR_RST_PCI1_BTN,
		.offset	= 0x4,
		.type	= RST_PCIE_BTN,
	},
	{
		.id	= SDR_RST_PCI_ICT,
		.offset	= 0x48,
		.type	= RST_MONO,
	},
	{
		.id	= SDR_RST_PCI0_PERSTN_PAD,
		.offset	= 0x100,
		.type	= RST_PCIE_PERSTN_PAD,
	},
	{
		.id	= SDR_RST_PCI1_PERSTN_PAD,
		.offset	= 0x100,
		.type	= RST_PCIE_PERSTN_PAD,
	},
};

struct mcom03_reset_priv {
	const struct reset_ops *ops;
	struct regmap *urb;
	unsigned long offset;
	unsigned int subsystem;
};

/*
 * Dummy request/free callbacks for reset controllers without per-clock
 * resources or requests. Subsystems with special requirements should
 * provide their own implementations.
 */
static int mcom03_reset_common_free(struct reset_ctl *rst)
{
	return 0;
}

static int mcom03_reset_common_request(struct reset_ctl *rst)
{
	return 0;
}

static const struct sdr_reset *mcom03_get_sdr_reset_by_id(u32 id)
{
	const struct sdr_reset *desc = NULL;

	for (int i = 0; i < ARRAY_SIZE(sdr_reset_map); i++) {
		if (sdr_reset_map[i].id == id) {
			desc = &sdr_reset_map[i];
			break;
		}
	}

	if (!desc)
		return ERR_PTR(-EOPNOTSUPP);

	return desc;
}

static int mcom03_reset_sdr_deassert(struct reset_ctl *rst)
{
	struct mcom03_reset_priv *priv = dev_get_priv(rst->dev);
	const struct sdr_reset *desc = mcom03_get_sdr_reset_by_id(rst->id);

	if (IS_ERR(desc))
		return PTR_ERR(desc);

	switch (desc->type) {
	case RST_MONO:
		return regmap_write(priv->urb,
				    priv->offset + desc->offset,
				    RST_MONO_ON);
	case RST_PCIE_BTN:
		return regmap_update_bits(priv->urb,
					  priv->offset + desc->offset,
					  PP_PCIE_BTN, PP_PCIE_BTN);
	case RST_PCIE_PERSTN_PAD:
		if (desc->id == SDR_RST_PCI0_PERSTN_PAD)
			return regmap_update_bits(priv->urb,
						  priv->offset + desc->offset,
						  SDR_PCI0_PERSTN_MODE | SDR_PCI0_PERSTN,
						  SDR_PCI0_PERSTN);
		if (desc->id == SDR_RST_PCI1_PERSTN_PAD)
			return regmap_update_bits(priv->urb,
						  priv->offset + desc->offset,
						  SDR_PCI1_PERSTN_MODE | SDR_PCI1_PERSTN,
						  SDR_PCI1_PERSTN);
	}

	return 0;
}

static int mcom03_reset_sdr_assert(struct reset_ctl *rst)
{
	struct mcom03_reset_priv *priv = dev_get_priv(rst->dev);
	const struct sdr_reset *desc = mcom03_get_sdr_reset_by_id(rst->id);

	if (IS_ERR(desc))
		return PTR_ERR(desc);

	switch (desc->type) {
	case RST_PCIE_BTN:
		return regmap_update_bits(priv->urb,
					  priv->offset + desc->offset,
					  PP_PCIE_BTN, 0);
	case RST_PCIE_PERSTN_PAD:
		if (desc->id == SDR_RST_PCI0_PERSTN_PAD)
			return regmap_update_bits(priv->urb,
						  priv->offset + desc->offset,
						  SDR_PCI0_PERSTN | SDR_PCI0_PERSTN_MODE, 0);
		if (desc->id == SDR_RST_PCI1_PERSTN_PAD)
			return regmap_update_bits(priv->urb,
						  priv->offset + desc->offset,
						  SDR_PCI1_PERSTN | SDR_PCI1_PERSTN_MODE, 0);
		break;
	case RST_MONO:
	default:
		/* Reset assertion of individual SDR subcomponents causes SDR subsystem
		 * freeze (see MCOM03-1943). Skip asserts, allow only deasserts.
		 */
		dev_dbg(rst->dev, "Skip reset of SDR subcomponent: %ld", rst->id);
	}

	return 0;
}

static const struct reset_ops mcom03_reset_sdr_ops = {
	.request	= mcom03_reset_common_request,
	.rfree		= mcom03_reset_common_free,
	.rst_assert	= mcom03_reset_sdr_assert,
	.rst_deassert	= mcom03_reset_sdr_deassert,
};

static int mcom03_reset_hsperiph_deassert(struct reset_ctl *rst)
{
	struct mcom03_reset_priv *priv = dev_get_priv(rst->dev);
	u32 mask = BIT(rst->id) | BIT(rst->id + WRITE_ENABLE_OFFSET);
	int ret;

	ret = regmap_update_bits(priv->urb, priv->offset, mask,
				 BIT(rst->id + WRITE_ENABLE_OFFSET));
	if (ret < 0)
		return ret;

	return 0;
}

static int mcom03_reset_hsperiph_assert(struct reset_ctl *rst)
{
	struct mcom03_reset_priv *priv = dev_get_priv(rst->dev);
	u32 mask = BIT(rst->id) | BIT(rst->id + WRITE_ENABLE_OFFSET);
	int ret;

	ret = regmap_update_bits(priv->urb, priv->offset, mask, mask);
	if (ret < 0)
		return ret;

	return 0;
}

static const struct reset_ops mcom03_reset_hsperiph_ops = {
	.request	= mcom03_reset_common_request,
	.rfree		= mcom03_reset_common_free,
	.rst_assert	= mcom03_reset_hsperiph_assert,
	.rst_deassert	= mcom03_reset_hsperiph_deassert,
};

static int mcom03_reset_deassert(struct reset_ctl *rst)
{
	struct mcom03_reset_priv *priv = dev_get_priv(rst->dev);

	return priv->ops->rst_deassert(rst);
}

static int mcom03_reset_assert(struct reset_ctl *rst)
{
	struct mcom03_reset_priv *priv = dev_get_priv(rst->dev);

	return priv->ops->rst_assert(rst);
}

static int mcom03_reset_free(struct reset_ctl *rst)
{
	struct mcom03_reset_priv *priv = dev_get_priv(rst->dev);

	return priv->ops->rfree(rst);
}

static int mcom03_reset_request(struct reset_ctl *rst)
{
	struct mcom03_reset_priv *priv = dev_get_priv(rst->dev);

	return priv->ops->request(rst);
}

/*
 * mcom03_reset_ops - Reset controller operations dispatcher.
 *
 * These functions act as wrappers that delegate the actual reset operations
 * to subsystem-specific implementation stored in driver's private data.
 * The specific ops are selected during probe() based on the "elvees,subsystem"
 * device tree property.
 */
static const struct reset_ops mcom03_reset_ops = {
	.request	= mcom03_reset_request,
	.rfree		= mcom03_reset_free,
	.rst_assert	= mcom03_reset_assert,
	.rst_deassert	= mcom03_reset_deassert,
};

static const struct udevice_id mcom03_reset_ids[] = {
	{ .compatible = "elvees,mcom03-reset" },
	{ /* sentinel */ }
};

static int mcom03_reset_probe(struct udevice *dev)
{
	struct mcom03_reset_priv *priv = dev_get_priv(dev);
	int ret;

	ret = dev_read_u32(dev, "elvees,subsystem", &priv->subsystem);
	if (ret)
		return ret;

	if (priv->subsystem >= MCOM03_SUBSYSTEM_MAX)
		return -EINVAL;

	priv->urb = syscon_get_regmap(dev->parent);

	if (IS_ERR(priv->urb))
		return PTR_ERR(priv->urb);

	priv->offset = devfdt_get_addr(dev);
	if (!priv->offset)
		return -ENOMEM;

	switch (priv->subsystem) {
	case MCOM03_SUBSYSTEM_SDR:
		priv->ops = &mcom03_reset_sdr_ops;
		break;
	case MCOM03_SUBSYSTEM_HSPERIPH:
		priv->ops = &mcom03_reset_hsperiph_ops;
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

U_BOOT_DRIVER(mcom03_reset) = {
	.name		= "mcom03-reset",
	.id		= UCLASS_RESET,
	.of_match	= mcom03_reset_ids,
	.ops		= &mcom03_reset_ops,
	.probe		= mcom03_reset_probe,
	.priv_auto	= sizeof(struct mcom03_reset_priv),
};
