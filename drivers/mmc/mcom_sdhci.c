/*
 * Copyright 2017 RnD Center "ELVEES", JSC
 *
 * ELVEES MCom SD Host Controller Interface
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <sdhci.h>

DECLARE_GLOBAL_DATA_PTR;

struct arasan_sdhci_plat {
	struct mmc_config cfg;
	struct mmc mmc;
};

static int arasan_sdhci_get_cd(struct sdhci_host *host)
{
	struct udevice *dev = host->mmc->dev;
	unsigned int status;

	if (fdtdec_get_bool(gd->fdt_blob, dev_of_offset(dev), "broken-cd")) {
		sdhci_writeb(host, SDHCI_CTRL_CD_TEST_INS | SDHCI_CTRL_CD_TEST,
			     SDHCI_HOST_CONTROL);

		status = sdhci_readl(host, SDHCI_PRESENT_STATE);
		while ((!(status & SDHCI_CARD_PRESENT)) ||
		       (!(status & SDHCI_CARD_STATE_STABLE)) ||
		       (!(status & SDHCI_CARD_DETECT_PIN_LEVEL)))
			status = sdhci_readl(host, SDHCI_PRESENT_STATE);
	}

	return 0;
}

static const struct sdhci_ops arasan_sdhci_ops = {
	.get_cd = arasan_sdhci_get_cd,
};

static int arasan_sdhci_bind(struct udevice *dev)
{
	struct arasan_sdhci_plat *plat = dev_get_platdata(dev);

	return sdhci_bind(dev, &plat->mmc, &plat->cfg);
}

static int arasan_sdhci_probe(struct udevice *dev)
{
	struct arasan_sdhci_plat *plat = dev_get_platdata(dev);
	struct mmc_uclass_priv *upriv = dev_get_uclass_priv(dev);
	struct sdhci_host *host = dev_get_priv(dev);
	int ret;

	host->name = dev->name;
	host->ioaddr = (void *)devfdt_get_addr(dev);
	host->quirks = SDHCI_QUIRK_WAIT_SEND_CMD | SDHCI_QUIRK_NO_HISPD_BIT;

	host->ops = &arasan_sdhci_ops;

	ret = sdhci_setup_cfg(&plat->cfg, host, SPLL_FREQ, 400000);
	if (ret)
		return ret;

	host->mmc = &plat->mmc;
	host->mmc->dev = dev;
	host->mmc->priv = host;
	upriv->mmc = host->mmc;

	return sdhci_probe(dev);
}

static const struct udevice_id arasan_sdhci_match_table[] = {
	{ .compatible = "arasan,sdhci-8.9a" },
	{ },
};

U_BOOT_DRIVER(arasan_sdhci_drv) = {
	.name = "arasan_sdhci",
	.id = UCLASS_MMC,
	.of_match = arasan_sdhci_match_table,
	.bind = arasan_sdhci_bind,
	.probe = arasan_sdhci_probe,
	.priv_auto_alloc_size = sizeof(struct sdhci_host),
	.platdata_auto_alloc_size = sizeof(struct arasan_sdhci_plat),
	.ops = &sdhci_ops,
};
