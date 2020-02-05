// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2020 RnD Center "ELVEES", JSC
 */

#include <asm/io.h>
#include <common.h>
#include <dm.h>
#include <reset-uclass.h>

#define WRITE_ENABLE_OFFSET	16

struct mcom03_reset_priv {
	void __iomem *base;
};

static int mcom03_reset_deassert(struct reset_ctl *rst)
{
	struct mcom03_reset_priv *priv = dev_get_priv(rst->dev);
	u32 val;

	val = readl(priv->base);
	val &= ~BIT(rst->id);
	val |= BIT(rst->id + WRITE_ENABLE_OFFSET);
	writel(val, priv->base);

	return 0;
}

static int mcom03_reset_assert(struct reset_ctl *rst)
{
	struct mcom03_reset_priv *priv = dev_get_priv(rst->dev);
	u32 val;

	val = readl(priv->base);
	val |= BIT(rst->id) | BIT(rst->id + WRITE_ENABLE_OFFSET);
	writel(val, priv->base);

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
	.free = mcom03_reset_free,
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

	priv->base = dev_remap_addr(dev);
	if (!priv->base)
		return -ENOMEM;

	return 0;
}

U_BOOT_DRIVER(mcom03_reset) = {
	.name = "mcom03-reset",
	.id = UCLASS_RESET,
	.of_match = mcom03_reset_ids,
	.ops = &mcom03_reset_reset_ops,
	.probe = mcom03_reset_probe,
	.priv_auto_alloc_size = sizeof(struct mcom03_reset_priv),
};
