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
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <mmc.h>
#include <power/regulator.h>
#include <sdhci.h>
#include <syscon.h>
#include <regmap.h>
#include <reset.h>

#define SDMMC_PADCFG(ctrl_id)			(0x2C + (ctrl_id) * 0x3C)
#define SDMMC_PADCFG_EN				BIT(0)

#define SDMMC_CLK_PADCFG(ctrl_id)		(0x30 + (ctrl_id) * 0x3C)
#define SDMMC_CMD_PADCFG(ctrl_id)		(0x34 + (ctrl_id) * 0x3C)
#define SDMMC_DAT_PADCFG(ctrl_id)		(0x38 + (ctrl_id) * 0x3C)
#define SDMMC_CLK_PADCFG_PU			BIT(1)

#define SDMMC_CORECFG1(ctrl_id)			(0x40 + (ctrl_id) * 0x3C)
#define SDMMC_CORECFG1_BASECLKFREQ		GENMASK(15, 8)
#define SDMMC_CORECFG1_HSEN			BIT(21)
#define SDMMC_CORECFG1_SLOT_NONREMOVABLE	BIT(30)

#define MISC_PADCFG				0x1a4
#define MISC_PADCFG_EN				BIT(8)

/* Debounce time for removable slot in Arasan SDMMC controller is 1024 ms. */
#define DEBOUNCE_TIMEOUT_US			1100000

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
	bool haps;
	struct regmap *soc_ctl_base;
};

static int mcom03_sdhci_set_soc_regs(struct udevice *dev)
{
	struct mcom03_sdhci_priv *priv = dev_get_priv(dev);
	struct regmap *soc_ctl_base = priv->soc_ctl_base;
	u32 freq;
	int ret;

	freq = priv->freq / 1000 / 1000;
	ret = regmap_update_bits(soc_ctl_base, SDMMC_CORECFG1(priv->ctrl_id),
				 SDMMC_CORECFG1_BASECLKFREQ,
				 SDMMC_SET_FIELD(SDMMC_CORECFG1_BASECLKFREQ,
						 freq));
	if (ret < 0)
		return ret;

	if (priv->broken_hs) {
		ret = regmap_update_bits(soc_ctl_base,
					 SDMMC_CORECFG1(priv->ctrl_id),
					 SDMMC_CORECFG1_HSEN, 0);
		if (ret)
			return ret;
	}

	ret = regmap_update_bits(soc_ctl_base, SDMMC_CMD_PADCFG(priv->ctrl_id),
				 SDMMC_CLK_PADCFG_PU, SDMMC_CLK_PADCFG_PU);
	if (ret)
		return ret;

	ret = regmap_update_bits(soc_ctl_base, SDMMC_DAT_PADCFG(priv->ctrl_id),
				 SDMMC_CLK_PADCFG_PU, SDMMC_CLK_PADCFG_PU);
	if (ret)
		return ret;

	ret = regmap_update_bits(soc_ctl_base, SDMMC_PADCFG(priv->ctrl_id),
				 SDMMC_PADCFG_EN, SDMMC_PADCFG_EN);
	if (ret)
		return ret;

	ret = regmap_update_bits(soc_ctl_base, MISC_PADCFG,
				 MISC_PADCFG_EN, MISC_PADCFG_EN);

	return ret;
}

/*
 * If slot set as removable then SDHCI_CARD_PRESENT bit will be set
 * after ~1 second after card insertion or reset deassertion.
 * This function waits debounce completion.
 */
static void mcom03_wait_card_detect_debounce(struct sdhci_host *host)
{
	const u32 mask = SDHCI_CARD_DETECT_PIN_LEVEL | SDHCI_CARD_PRESENT;
	u32 ret;
	u32 val;

	ret = readl_poll_timeout(host->ioaddr + SDHCI_PRESENT_STATE,
				 val,
				 (val & mask) != SDHCI_CARD_DETECT_PIN_LEVEL,
				 DEBOUNCE_TIMEOUT_US);

	if (ret)
		printf("SD card detect debounce timeout\n");
}

/* TODO: Use manual voltage switching using sdmmcX_padcfg. See rf#14673. */
#if CONFIG_IS_ENABLED(MMC_IO_VOLTAGE)
static void mcom03_sdhci_set_18v(struct sdhci_host *host, bool enable)
{
	u32 reg;

	reg = sdhci_readw(host, SDHCI_HOST_CONTROL2);

	/* Check if required voltage is already set */
	if (enable == !!(reg & SDHCI_CTRL_VDD_180))
		return;

	if (enable)
		reg |= SDHCI_CTRL_VDD_180;
	else
		reg &= ~SDHCI_CTRL_VDD_180;

	sdhci_writew(host, reg, SDHCI_HOST_CONTROL2);

	/* According to SDHCI specification external regulator output shall be
	 * stable within 5 ms. */
	mdelay(5);

	/* Don't check register value as required by SDHCI specification because
	 * MCom-03 doesn't have error detection logic for signaling voltage
	 * switch. */
}

/*
 * Common MMC code doesn't take into account voltage regulators. Drivers should
 * take care on its own.
 */
static int mcom03_sdhci_set_ios_post(struct sdhci_host *host)
{
	const struct mmc *mmc = host->mmc;
	const int voltage = mmc_voltage_to_mv(mmc->signal_voltage) * 1000;
	int ret;

	if (mmc->signal_voltage == MMC_SIGNAL_VOLTAGE_120)
		return -ENOTSUPP;

#if CONFIG_IS_ENABLED(DM_REGULATOR)
	if (mmc->vqmmc_supply) {
		/* U-Boot doesn't support set_value() for fixed regulator */
		int old = regulator_get_value(mmc->vqmmc_supply);

		if (old != voltage) {
			ret = regulator_set_value(mmc->vqmmc_supply, voltage);
			if (ret)
				return ret;
		}

		ret = regulator_set_enable_if_allowed(mmc->vqmmc_supply, true);
		if (ret)
			return ret;
	}
#endif

	mcom03_sdhci_set_18v(host,
			     mmc->signal_voltage == MMC_SIGNAL_VOLTAGE_180);

	/* TODO: Decide what to do in case of MMC_SIGNAL_VOLTAGE_000. */

	return 0;
}
#else
static int mcom03_sdhci_set_ios_post(struct sdhci_host *host)
{
	return 0;
}
#endif

const struct sdhci_ops mcom03_sdhci_ops = {
	.set_ios_post = mcom03_sdhci_set_ios_post
};

static int mcom03_sdhci_bind(struct udevice *dev)
{
	struct mcom03_sdhci_plat *plat = dev_get_platdata(dev);

	return sdhci_bind(dev, &plat->mmc, &plat->cfg);
}

static int mcom03_sdhci_ofdata_to_platdata(struct udevice *dev)
{
	struct mcom03_sdhci_priv *priv = dev_get_priv(dev);
	struct ofnode_phandle_args args;
	int ret;

	priv->host.name = dev->name;

	priv->host.ioaddr = (void *)dev_read_addr(dev);
	if (IS_ERR(priv->host.ioaddr))
		return PTR_ERR(priv->host.ioaddr);

	priv->host.ops = &mcom03_sdhci_ops;

	ret = dev_read_phandle_with_args(dev, "arasan,soc-ctl-syscon", NULL,
					 0, 0, &args);
	if (ret)
		return ret;

	priv->soc_ctl_base = syscon_node_to_regmap(args.node);
	if (IS_ERR(priv->soc_ctl_base))
		return PTR_ERR(priv->soc_ctl_base);

	ret = dev_read_u32(dev, "elvees,ctrl-id", &priv->ctrl_id);
	if (ret < 0)
		return ret;

	priv->broken_hs = dev_read_bool(dev, "elvees,broken-hs");
	priv->haps = dev_read_bool(dev, "elvees,haps");

	return 0;
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

	ret = mmc_of_parse(dev, &plat->cfg);
	if (ret)
		goto assert_reset;

	if (priv->haps)
		mdelay(100);  /* TODO: 100 ms may be too much */

	ret = mcom03_sdhci_set_soc_regs(dev);
	if (ret)
		goto assert_reset;

	host->name = dev->name;
	host->max_clk = priv->freq;

	host->quirks = SDHCI_QUIRK_WAIT_SEND_CMD;
	if (priv->broken_hs)
		host->quirks |= SDHCI_QUIRK_BROKEN_HISPD_MODE;

	host->mmc = &plat->mmc;
	host->mmc->dev = dev;
	host->mmc->priv = &priv->host;
	upriv->mmc = host->mmc;

	/* plat->cfg.f_max is filled early in mmc_of_parse() */
	ret = sdhci_setup_cfg(&plat->cfg, host, plat->cfg.f_max, 400000);
	if (ret)
		goto assert_reset;

	mcom03_wait_card_detect_debounce(host);

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
	.ofdata_to_platdata = mcom03_sdhci_ofdata_to_platdata,
	.bind = mcom03_sdhci_bind,
	.probe = mcom03_sdhci_probe,
	.priv_auto_alloc_size = sizeof(struct mcom03_sdhci_priv),
	.platdata_auto_alloc_size = sizeof(struct mcom03_sdhci_plat),
	.ops = &sdhci_ops,
};
