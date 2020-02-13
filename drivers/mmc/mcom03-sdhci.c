// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2020 RnD Center "ELVEES", JSC
 *
 * Arasan SDHCI support for ELVEES MCom-03 SoC
 */
#include <clk.h>
#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <linux/bitfield.h>
#include <sdhci.h>
#include <syscon.h>
#include <regmap.h>
#include <reset.h>

#define SDMMC_PADCFG(ctrl_id)			(0x2C + (ctrl_id) * 0x3C)
#define SDMMC_PADCFG_EN				BIT(0)

#define SDMMC_CORECFG1(ctrl_id)			(0x40 + (ctrl_id) * 0x3C)
#define SDMMC_CORECFG1_BASECLKFREQ		GENMASK(15, 8)
#define SDMMC_CORECFG1_HSEN			BIT(21)
#define SDMMC_CORECFG1_SLOT_NONREMOVABLE	BIT(30)

#define SDMMC_SET_FIELD(m, v)			((v) << (ffs(m) - 1))

DECLARE_GLOBAL_DATA_PTR;

struct mcom03_sdhci_plat {
	struct mmc_config cfg;
	struct mmc mmc;
};

struct mcom03_sdhci_priv {
	struct sdhci_host host;
	u32 freq;
	u32 ctrl_id;
	bool broken_hs;
	bool non_removable;
};

static int mcom03_sdhci_bind(struct udevice *dev)
{
	struct mcom03_sdhci_plat *plat = dev_get_platdata(dev);

	return sdhci_bind(dev, &plat->mmc, &plat->cfg);
}

static int mcom03_sdhci_of_parse(struct udevice *dev)
{
	struct mcom03_sdhci_priv *priv = dev_get_priv(dev);
	struct mcom03_sdhci_plat *plat = dev_get_platdata(dev);
	int ret;

	priv->host.ioaddr = (void *)devfdt_get_addr(dev);

	ret = dev_read_u32(dev, "elvees,ctrl-id", &priv->ctrl_id);
	if (ret < 0)
		return ret;

	priv->broken_hs = dev_read_bool(dev, "elvees,broken-hs");

	/* Remove it after U-Boot will be updated at least to 2019.10 */
	priv->non_removable = dev_read_bool(dev, "non-removable");

	return mmc_of_parse(dev, &plat->cfg);
}

static int mcom03_sdhci_set_soc_regs(struct udevice *dev)
{
	struct mcom03_sdhci_priv *priv = dev_get_priv(dev);
	struct regmap *regmap;
	struct ofnode_phandle_args args;
	u32 freq;
	int ret;

	ret = dev_read_phandle_with_args(dev, "arasan,soc-ctl-syscon", NULL,
					 0, 0, &args);
	if (ret)
		return ret;

	regmap = syscon_node_to_regmap(args.node);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	freq = priv->freq / 1000 / 1000;
	ret = regmap_update_bits(regmap, SDMMC_CORECFG1(priv->ctrl_id),
				 SDMMC_CORECFG1_BASECLKFREQ,
				 SDMMC_SET_FIELD(SDMMC_CORECFG1_BASECLKFREQ,
						 freq));
	if (ret < 0)
		return ret;

	if (priv->broken_hs) {
		ret = regmap_update_bits(regmap, SDMMC_CORECFG1(priv->ctrl_id),
					 SDMMC_CORECFG1_HSEN, 0);
		if (ret)
			return ret;
	}

	if (priv->non_removable) {
		ret = regmap_update_bits(regmap, SDMMC_CORECFG1(priv->ctrl_id),
					 SDMMC_CORECFG1_SLOT_NONREMOVABLE,
					 SDMMC_CORECFG1_SLOT_NONREMOVABLE);
		if (ret)
			return ret;
	}

	ret = regmap_update_bits(regmap, SDMMC_PADCFG(priv->ctrl_id),
				 SDMMC_PADCFG_EN, SDMMC_PADCFG_EN);

	return ret;
}

static int mcom03_sdhci_probe(struct udevice *dev)
{
	struct mcom03_sdhci_plat *plat = dev_get_platdata(dev);
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct mcom03_sdhci_priv *priv = dev_get_priv(dev);
	struct sdhci_host *host = &priv->host;
	struct reset_ctl rst_ctl;
	struct clk clk;
	int ret;

	ret = clk_get_by_index(dev, 0, &clk);
	if (ret < 0)
		return ret;

	priv->freq = clk_get_rate(&clk);
	if (IS_ERR_VALUE(priv->freq))
		return priv->freq;

	ret = clk_enable(&clk);
	if (ret < 0 && ret != -ENOSYS)
		return ret;

	ret = reset_get_by_index(dev, 0, &rst_ctl);
	if (ret < 0)
		goto disable_clk;

	ret = reset_deassert(&rst_ctl);
	if (ret < 0)
		goto disable_clk;

	/* TODO: 100 ms may be too much */
	mdelay(100);

	ret = mcom03_sdhci_of_parse(dev);
	if (ret)
		goto assert_reset;

	ret = mcom03_sdhci_set_soc_regs(dev);
	if (ret)
		goto assert_reset;

	host->name = dev->name;
	host->max_clk = priv->freq;

	host->quirks = SDHCI_QUIRK_WAIT_SEND_CMD;
	if (priv->broken_hs)
		host->quirks |= SDHCI_QUIRK_BROKEN_HISPD_MODE;

	ret = sdhci_setup_cfg(&plat->cfg, host, 0, 400000);
	if (ret)
		goto assert_reset;

	host->mmc = &plat->mmc;
	host->mmc->dev = dev;
	host->mmc->priv = &priv->host;
	upriv->mmc = host->mmc;

	return sdhci_probe(dev);

assert_reset:
	ret = reset_assert(&rst_ctl);

disable_clk:
	clk_disable(&clk);
	return ret;
}

static const struct udevice_id mcom03_sdhci_match_table[] = {
	{ .compatible = "elvees,mcom03-sdhci-8.9a" },
	{ /* sentinel */ },
};

U_BOOT_DRIVER(mcom03_sdhci_drv) = {
	.name = "mcom03-sdhci",
	.id = UCLASS_MMC,
	.of_match = mcom03_sdhci_match_table,
	.bind = mcom03_sdhci_bind,
	.probe = mcom03_sdhci_probe,
	.priv_auto_alloc_size = sizeof(struct mcom03_sdhci_priv),
	.platdata_auto_alloc_size = sizeof(struct mcom03_sdhci_plat),
	.ops = &sdhci_ops,
};
