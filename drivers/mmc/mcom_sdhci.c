/*
 * Copyright 2017 RnD Center "ELVEES", JSC
 *
 * ELVEES MCom SD Host Controller Interface
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <dm.h>
#include <linux/bitfield.h>

#include <fdtdec.h>
#include <sdhci.h>

DECLARE_GLOBAL_DATA_PTR;

#define SDMMC_INIT_CONFIG_1	0x100
#define SDMMC_INIT_CONFIG_ITAPDLYSEL	GENMASK(31, 27)
#define SDMMC_INIT_CONFIG_ITAPDLYENA	BIT(26)
#define SDMMC_INIT_CONFIG_ITAPCHGWIN	BIT(25)
#define SDMMC_INIT_CONFIG_OTAPDLYSEL	GENMASK(24, 21)
#define SDMMC_INIT_CONFIG_OTAPDLYENA	BIT(20)

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

static void arasan_sdhci_set_tapdelay(struct sdhci_host *host)
{
	struct mmc *mmc = host->mmc;
	struct udevice *dev = mmc->dev;
	u32 reg;
	s32 clk_phase[2] = { -1, -1 };

	switch (mmc->selected_mode) {
	case MMC_HS:
	case MMC_HS_52:
		if (dev_read_prop(dev, "mcom02,clk-phase-mmc-hs", NULL))
			dev_read_u32_array(dev, "clk-phase-mmc-hs",
					   (u32 *)clk_phase, 2);
		break;
	case SD_HS:
		if (dev_read_prop(dev, "mcom02,clk-phase-sd-hs", NULL))
			dev_read_u32_array(dev, "clk-phase-sd-hs",
					   (u32 *)clk_phase, 2);
		break;
	case MMC_DDR_52:
		if (dev_read_prop(dev, "mcom02,clk-phase-mmc-ddr52", NULL))
			dev_read_u32_array(dev, "clk-phase-mmc-ddr52",
					   (u32 *)clk_phase, 2);
		break;
	case UHS_SDR12:
		if (dev_read_prop(dev, "mcom02,clk-phase-uhs-sdr12", NULL))
			dev_read_u32_array(dev, "clk-phase-uhs-sdr12",
					   (u32 *)clk_phase, 2);
		break;
	case UHS_SDR25:
		if (dev_read_prop(dev, "mcom02,clk-phase-uhs-sdr25", NULL))
			dev_read_u32_array(dev, "clk-phase-uhs-sdr25",
					   (u32 *)clk_phase, 2);
		break;
	case UHS_SDR50:
		if (dev_read_prop(dev, "mcom02,clk-phase-uhs-sdr50", NULL))
			dev_read_u32_array(dev, "clk-phase-uhs-sdr50",
					   (u32 *)clk_phase, 2);
		break;
	case UHS_DDR50:
		if (dev_read_prop(dev, "mcom02,clk-phase-uhs-ddr50", NULL))
			dev_read_u32_array(dev, "clk-phase-uhs-ddr50",
					   (u32 *)clk_phase, 2);
		break;
	case UHS_SDR104:
		if (dev_read_prop(dev, "mcom02,clk-phase-uhs-sdr104", NULL))
			dev_read_u32_array(dev, "clk-phase-uhs-sdr104",
					   (u32 *)clk_phase, 2);
		break;
	case MMC_HS_200:
		if (dev_read_prop(dev, "mcom02,clk-phase-mmc-hs200", NULL))
			dev_read_u32_array(dev, "clk-phase-mmc-hs200",
					   (u32 *)clk_phase, 2);
		break;
	default:
		break;
	}

	/* Set Tap Delay Lines */
	reg = sdhci_readl(host, SDMMC_INIT_CONFIG_1);
	if (clk_phase[0] != -1) {
		/* This signal should be asserted few clocks before
		 * the corectrl_itapdlysel changes and should be
		 * asserted for few clocks after.
		 */
		reg |= SDMMC_INIT_CONFIG_ITAPCHGWIN;
		sdhci_writel(host, reg, SDMMC_INIT_CONFIG_1);

		reg &= ~SDMMC_INIT_CONFIG_ITAPDLYSEL;
		reg |= FIELD_PREP(SDMMC_INIT_CONFIG_ITAPDLYSEL, clk_phase[0]);
		reg |= SDMMC_INIT_CONFIG_ITAPDLYENA;
		udelay(1); /* Asserted few clocks before */
		sdhci_writel(host, reg, SDMMC_INIT_CONFIG_1);

		reg &= ~SDMMC_INIT_CONFIG_ITAPCHGWIN;
		udelay(1); /* Asserted few clocks after */
		sdhci_writel(host, reg, SDMMC_INIT_CONFIG_1);
	} else if (reg & SDMMC_INIT_CONFIG_ITAPDLYENA) {
		reg &= ~SDMMC_INIT_CONFIG_ITAPDLYENA;
		sdhci_writel(host, reg, SDMMC_INIT_CONFIG_1);
	}

	reg = sdhci_readl(host, SDMMC_INIT_CONFIG_1);
	if (clk_phase[1] != -1) {
		reg &= ~SDMMC_INIT_CONFIG_OTAPDLYSEL;
		reg |= FIELD_PREP(SDMMC_INIT_CONFIG_OTAPDLYSEL, clk_phase[1]);
		reg |= SDMMC_INIT_CONFIG_OTAPDLYENA;
		sdhci_writel(host, reg, SDMMC_INIT_CONFIG_1);
	} else if (reg & SDMMC_INIT_CONFIG_OTAPDLYENA) {
		reg &= ~SDMMC_INIT_CONFIG_OTAPDLYENA;
		sdhci_writel(host, reg, SDMMC_INIT_CONFIG_1);
	}
}

static const struct sdhci_ops arasan_sdhci_ops = {
	.get_cd = arasan_sdhci_get_cd,
	.set_delay = arasan_sdhci_set_tapdelay,
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
	host->max_clk = SPLL_FREQ;
	host->ops = &arasan_sdhci_ops;

	ret = sdhci_setup_cfg(&plat->cfg, host, 0, 400000);
	if (ret)
		return ret;

	host->mmc = &plat->mmc;
	host->mmc->dev = dev;
	host->mmc->priv = host;
	upriv->mmc = host->mmc;

	return sdhci_probe(dev);
}

static const struct udevice_id arasan_sdhci_match_table[] = {
	{ .compatible = "elvees,sdhci-mcom02" },
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
