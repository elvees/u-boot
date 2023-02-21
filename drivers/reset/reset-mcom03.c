// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2020 RnD Center "ELVEES", JSC
 */

#include <asm/io.h>
#include <common.h>
#include <dm.h>
#include <reset-uclass.h>
#include <syscon.h>
#include <regmap.h>
#include <dt-bindings/soc/elvees,mcom03.h>

#define WRITE_ENABLE_OFFSET	16

struct mcom03_reset_priv {
	struct regmap *urb;
	unsigned long offset;
	unsigned int subsystem;
};

static int mcom03_reset_deassert(struct reset_ctl *rst)
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

static int mcom03_reset_assert(struct reset_ctl *rst)
{
	struct mcom03_reset_priv *priv = dev_get_priv(rst->dev);
	u32 mask = BIT(rst->id) | BIT(rst->id + WRITE_ENABLE_OFFSET);
	int ret;

	ret = regmap_update_bits(priv->urb, priv->offset, mask, mask);
	if (ret < 0)
		return ret;

	return 0;
}

static int mcom03_reset_free(struct reset_ctl *rst)
{
	return 0;
}

static int mcom03_reset_request(struct reset_ctl *rst)
{
	return 0;
}

static const struct reset_ops mcom03_reset_reset_ops = {
	.request = mcom03_reset_request,
	.rfree = mcom03_reset_free,
	.rst_assert = mcom03_reset_assert,
	.rst_deassert = mcom03_reset_deassert,
};

static const struct udevice_id mcom03_reset_ids[] = {
	{ .compatible = "elvees,mcom03-reset" },
	{ /* sentinel */ }
};

static int mcom03_reset_probe(struct udevice *dev)
{
	struct mcom03_reset_priv *priv = dev_get_priv(dev);
	int ret;

	priv->urb = syscon_get_regmap(dev->parent);

	if (IS_ERR(priv->urb))
		return PTR_ERR(priv->urb);

	priv->offset = devfdt_get_addr(dev);
	if (!priv->offset)
		return -ENOMEM;

	ret = dev_read_u32(dev, "elvees,subsystem", &priv->subsystem);
	if (ret)
		return ret;

	if (priv->subsystem != MCOM03_SUBSYSTEM_HSPERIPH)
		return -EINVAL;

	return 0;
}

U_BOOT_DRIVER(mcom03_reset) = {
	.name = "mcom03-reset",
	.id = UCLASS_RESET,
	.of_match = mcom03_reset_ids,
	.ops = &mcom03_reset_reset_ops,
	.probe = mcom03_reset_probe,
	.priv_auto = sizeof(struct mcom03_reset_priv),
};
