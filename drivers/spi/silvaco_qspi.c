// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for Silvaco AXI QSPI controller on MCom-03.
 * Copyright 2020-2021 RnD Center "ELVEES", JSC
 */

#include <dm.h>
#include <errno.h>
#include <spi.h>
#include <clk.h>
#include <reset.h>
#include <regmap.h>
#include <syscon.h>
#include <asm/io.h>
#include <wait_bit.h>

#define SILVACO_CTRL_XFER	BIT(0)
#define SILVACO_CTRL_MSB1ST	BIT(2)
#define SILVACO_CTRL_MASTER	BIT(5)

#define SILVACO_AUX_INHDIN	BIT(3)
#define SILVACO_AUX_CNTXFEREXT	BIT(7)
#define SILVACO_AUX_BITSIZE(x)	(((x) - 1) << 8)

#define SILVACO_STAT_TXFULL	BIT(4)
#define SILVACO_STAT_RXEMPTY	BIT(5)

#define QSPI1_PAD		0x1c
#define QSPI1_PAD_EN		1
#define QSPI1_PAD_1V8		BIT(1)

#define QSPI1_SS		0x20
#define QSPI1_SISO		0x24
#define QSPI1_SCLK		0x28
#define QSPI1_PAD_CTL_MASK	GENMASK(10, 5)
#define QSPI1_PAD_CTL(x)	((x) << 5)
#define HS_URB_MAX_OUTPUT_8MA	0xF
#define QSPI1_PAD_PU		BIT(1)

struct silvaco_qspi_regs {
	u32 tx_data;      /* Serial Transmit Data Register */
	u32 rx_data;      /* Serial Receive Data Register */
	u32 reserv;       /* Reserved */
	u32 ctrl;         /* Primary Control Register */
	u32 ctrl_aux;     /* Auxiliary Control Register */
	u32 stat;         /* Status Register */
	u32 ss;           /* Slave Select Register */
	u32 ss_polar;     /* Slave Select Polarity Register */
	u32 intr_en;      /* Interrupt Enable Register */
	u32 intr_stat;    /* Interrupt Status Register */
	u32 inet_clr;     /* Interrupt Clear Register */
	u32 tx_fifo_lvl;  /* Transmit FIFO Fill Level Register (RO) */
	u32 rx_fifo_lvl;  /* Receive FIFO Fill Level Register (RO) */
	u32 unused;       /* Reserved */
	u32 master_delay; /* Master Mode Inter-transfer Delay Register */
	u32 en;           /* Enable/Disable Register */
	u32 gpo_set;      /* General Purpose Outputs Set Register */
	u32 gpo_clr;      /* General Purpose Outputs Clear Register */
	u32 fifo_depth;   /* Configured FIFO Depth Register (RO) */
	u32 fifo_wmark;   /* TX/RX Watermark Level Register */
	u32 tx_dummy;     /* TX FIFO Dummy Load Register */
};

struct silvaco_qspi_platdata {
	struct silvaco_qspi_regs *regs;
};

struct silvaco_qspi_priv {
	struct silvaco_qspi_regs *regs;
	struct clk clk_axi;
	struct clk clk_ext;
	struct reset_ctl rst_ctl;
	unsigned int freq;
};

static int silvaco_qspi_ofdata_to_platdata(struct udevice *dev)
{
	struct silvaco_qspi_platdata *plat = dev_get_platdata(dev);

	plat->regs = map_physmem(dev_read_addr(dev),
				 sizeof(struct silvaco_qspi_regs),
				 MAP_NOCACHE);

	return 0;
}

static int mcom03_qspi_set_soc_regs(struct udevice *dev)
{
	struct regmap *regmap;
	struct ofnode_phandle_args args;
	int ret;

	ret = dev_read_phandle_with_args(dev, "elvees,urb", NULL,
					 0, 0, &args);
	if (ret)
		return ret;

	regmap = syscon_node_to_regmap(args.node);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	ret = regmap_update_bits(regmap, QSPI1_PAD, QSPI1_PAD_1V8,
				 QSPI1_PAD_1V8);
	if (ret < 0)
		return ret;
	ret = regmap_update_bits(regmap, QSPI1_PAD, QSPI1_PAD_EN, QSPI1_PAD_EN);
	if (ret < 0)
		return ret;

	ret = regmap_update_bits(regmap, QSPI1_SS, QSPI1_PAD_CTL_MASK,
				 QSPI1_PAD_CTL(HS_URB_MAX_OUTPUT_8MA));
	if (ret < 0)
		return ret;
	ret = regmap_update_bits(regmap, QSPI1_SS, QSPI1_PAD_PU, QSPI1_PAD_PU);
	if (ret < 0)
		return ret;

	ret = regmap_update_bits(regmap, QSPI1_SISO, QSPI1_PAD_CTL_MASK,
				 QSPI1_PAD_CTL(HS_URB_MAX_OUTPUT_8MA));
	if (ret < 0)
		return ret;
	ret = regmap_update_bits(regmap, QSPI1_SISO, QSPI1_PAD_PU,
				 QSPI1_PAD_PU);
	if (ret < 0)
		return ret;

	ret = regmap_update_bits(regmap, QSPI1_SCLK, QSPI1_PAD_CTL_MASK,
				 QSPI1_PAD_CTL(HS_URB_MAX_OUTPUT_8MA));
	if (ret < 0)
		return ret;
	ret = regmap_update_bits(regmap, QSPI1_SCLK, QSPI1_PAD_PU,
				 QSPI1_PAD_PU);

	return ret;
}

static int silvaco_qspi_probe(struct udevice *dev)
{
	struct silvaco_qspi_priv *priv = dev_get_priv(dev);
	struct silvaco_qspi_platdata *plat = dev_get_platdata(dev);
	struct silvaco_qspi_regs *regs = plat->regs;
	int ret;

	priv->regs = plat->regs;

	ret = clk_get_by_name(dev, "clk_axi", &priv->clk_axi);
	if (ret < 0)
		return ret;

	ret = clk_enable(&priv->clk_axi);
	/* TODO: remove ENOSYS when clock driver will be implemented */
	if (ret && ret != -ENOSYS)
		return ret;

	ret = clk_get_by_name(dev, "clk_ext", &priv->clk_ext);
	if (ret < 0)
		goto disable_clk_axi;

	ret = clk_enable(&priv->clk_ext);
	/* TODO: remove ENOSYS when clock driver will be implemented */
	if (ret && ret != -ENOSYS)
		goto disable_clk_axi;

	priv->freq = clk_get_rate(&priv->clk_ext);
	if (IS_ERR_VALUE(priv->freq)) {
		ret = priv->freq;
		goto disable_clk_ext;
	}

	ret = reset_get_by_index(dev, 0, &priv->rst_ctl);
	if (ret < 0)
		goto disable_clk_ext;

	mdelay(10);

	ret = reset_deassert(&priv->rst_ctl);
	if (ret < 0)
		goto disable_clk_ext;

	ret = mcom03_qspi_set_soc_regs(dev);
	if (ret)
		goto assert_reset;

	writel(0, &regs->en);
	ret = wait_for_bit_le32(&regs->en, 0x1, false, 1000, false);
	if (ret)
		goto assert_reset;

	writel(SILVACO_CTRL_MASTER | SILVACO_CTRL_XFER | SILVACO_CTRL_MSB1ST,
	       &regs->ctrl);

	writel(SILVACO_AUX_BITSIZE(8) | SILVACO_AUX_INHDIN |
	       SILVACO_AUX_CNTXFEREXT, &regs->ctrl_aux);

	writel(1, &regs->en);
	ret = wait_for_bit_le32(&regs->en, 0x1, true, 1000, false);
	if (ret)
		goto assert_reset;

	/* Allow SISO1 and SISO0 out of tristate for normal full-duplex */
	writel(0xC << 4, &regs->gpo_set);
	writel(0x3 << 4, &regs->gpo_clr);

	/* full-duplex on */
	writel(BIT(12), &regs->gpo_clr);
	return 0;

assert_reset:
	reset_assert(&priv->rst_ctl);
	reset_free(&priv->rst_ctl);
disable_clk_ext:
	clk_disable(&priv->clk_ext);
	clk_free(&priv->clk_ext);
disable_clk_axi:
	clk_disable(&priv->clk_axi);
	clk_free(&priv->clk_axi);

	return ret;
}

static int silvaco_qspi_remove(struct udevice *dev)
{
	struct silvaco_qspi_priv *priv = dev_get_priv(dev);

	reset_assert(&priv->rst_ctl);
	reset_free(&priv->rst_ctl);

	clk_disable(&priv->clk_ext);
	clk_free(&priv->clk_ext);

	clk_disable(&priv->clk_axi);
	clk_free(&priv->clk_axi);

	return 0;
}

static int silvaco_qspi_claim_bus(struct udevice *dev)
{
	return 0;
}

static int silvaco_qspi_release_bus(struct udevice *dev)
{
	return 0;
}

static int silvaco_qspi_xfer(struct udevice *dev, unsigned int bitlen,
			     const void *dout, void *din, unsigned long flags)
{
	struct silvaco_qspi_priv *priv = dev_get_priv(dev->parent);
	struct dm_spi_slave_platdata *slave_plat = dev_get_parent_platdata(dev);
	struct silvaco_qspi_regs *regs = priv->regs;
	unsigned int bytes = bitlen / 8;
	const unsigned char *txp = dout;
	unsigned char *rxp = din;
	int ret = 0;
	u32 data;

	debug("%s: bus:%i cs:%i bitlen:%i bytes:%i flags:%lx\n", __func__,
	      dev->parent->seq, slave_plat->cs, bitlen, bytes, flags);

	/*
	 * The controller can do non-multiple-of-8 bit
	 * transfers, but this driver currently doesn't support it.
	 */
	if (bitlen % 8 != 0) {
		debug("%s: Non-byte-aligned transfer\n", __func__);
		ret = -EINVAL;
		flags |= SPI_XFER_END;
		goto done;
	}

	if (flags & SPI_XFER_BEGIN) {
		writel(BIT(slave_plat->cs), &regs->ss);
		writel(readl(&regs->ctrl_aux) & ~SILVACO_AUX_INHDIN,
		       &regs->ctrl_aux);
	}

	while (bytes--) {
		if (txp)
			data = *txp++;
		else
			data = 0xff;

		ret = wait_for_bit_le32(&regs->stat, SILVACO_STAT_TXFULL,
					false, 1000, false);
		if (ret) {
			debug("%s: Transmit timed out\n", __func__);
			flags |= SPI_XFER_END;
			goto done;
		}

		debug("%s: tx:%x\n", __func__, data);
		writel(data, &regs->tx_data);

		ret = wait_for_bit_le32(&regs->stat, SILVACO_STAT_RXEMPTY,
					false, 1000, false);
		if (ret) {
			debug("%s: Receive timed out\n", __func__);
			flags |= SPI_XFER_END;
			goto done;
		}

		data = readl(&regs->rx_data);
		if (rxp)
			*rxp++ = data & 0xff;

		debug("%s: rx:%x\n", __func__, data);
	}

done:
	if (flags & SPI_XFER_END) {
		writel(0, &regs->ss);
		writel(readl(&regs->ctrl_aux) | SILVACO_AUX_INHDIN,
		       &regs->ctrl_aux);
	}

	return ret;
}

static int silvaco_qspi_set_speed(struct udevice *dev, uint speed)
{
	return 0;
}

static int silvaco_qspi_set_mode(struct udevice *dev, uint mode)
{
	return 0;
}

static const struct dm_spi_ops silvaco_qspi_ops = {
	.claim_bus	= silvaco_qspi_claim_bus,
	.release_bus	= silvaco_qspi_release_bus,
	.xfer		= silvaco_qspi_xfer,
	.set_speed	= silvaco_qspi_set_speed,
	.set_mode	= silvaco_qspi_set_mode,
	/*
	 * cs_info is not needed, since we require all chip selects to be
	 * in the device tree explicitly
	 */
};

static const struct udevice_id silvaco_qspi_ids[] = {
	{ .compatible = "silvaco,axi-qspi" },
	{ }
};

U_BOOT_DRIVER(silvaco_qspi) = {
	.name	= "silvaco_qspi",
	.id	= UCLASS_SPI,
	.of_match = silvaco_qspi_ids,
	.ops	= &silvaco_qspi_ops,
	.ofdata_to_platdata = silvaco_qspi_ofdata_to_platdata,
	.platdata_auto_alloc_size = sizeof(struct silvaco_qspi_platdata),
	.priv_auto_alloc_size = sizeof(struct silvaco_qspi_priv),
	.probe	= silvaco_qspi_probe,
	.remove	= silvaco_qspi_remove,
};
