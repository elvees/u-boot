// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for Silvaco AXI QSPI controller on MCom-03.
 * Copyright 2020-2021 RnD Center "ELVEES", JSC
 */

#include <dm.h>
#include <errno.h>
#include <spi.h>
#include <spi-mem.h>
#include <clk.h>
#include <reset.h>
#include <regmap.h>
#include <syscon.h>
#include <wait_bit.h>
#include <asm/io.h>

#define SILVACO_CTRL_XFER	BIT(0)
#define SILVACO_CTRL_MSB1ST	BIT(2)
#define SILVACO_CTRL_MASTER	BIT(5)

#define SILVACO_AUX_INHDIN	BIT(3)
#define SILVACO_AUX_CNTXFEREXT	BIT(7)
#define SILVACO_AUX_BITSIZE(x)	(((x) - 1) << 8)

#define SILVACO_STAT_XFERIP	BIT(0)
#define SILVACO_STAT_TXEMPTY	BIT(2)
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

#define SILVACO_DUMMY_BYTES	0xff

#define GPO_OUTPUT_SISO_0_1	(0x3)
#define GPO_OUTPUT_SISO_2_3	(0xC)
#define GPO_TRISTATE_SISO_0_1	(0x3 << 4)
#define GPO_TRISTATE_SISO_2_3	(0xC << 4)
#define GPO_ENABLE_SISO_0_1	(0x3 << 8)
#define GPO_ENABLE_SISO_2_3	(0xC << 8)
#define GPO_DISABLE_FULL_DUPLEX	BIT(12)

/* IO lines */
#define SILVACO_NORMAL		1
#define SILVACO_QUAD		4

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
	u8 io_lines;
};

static int silvaco_qspi_ofdata_to_platdata(struct udevice *dev)
{
	struct silvaco_qspi_platdata *plat = dev_get_plat(dev);

	plat->regs = map_physmem(dev_read_addr(dev),
				 sizeof(struct silvaco_qspi_regs),
				 MAP_NOCACHE);

	return 0;
}

static int mcom03_qspi_set_soc_regs(struct udevice *dev)
{
	struct regmap *regmap;
	struct ofnode_phandle_args args;
	int devnum, ret;

	ret = dev_read_phandle_with_args(dev, "elvees,urb", NULL,
					 0, 0, &args);
	if (ret)
		return ret;

	regmap = syscon_node_to_regmap(args.node);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	dev_read_alias_seq(dev, &devnum);

	if (devnum == 1) {
		bool is_18v = dev_read_bool(dev, "elvees,pads-1v8-en");

		ret = regmap_update_bits(regmap, QSPI1_PAD, QSPI1_PAD_1V8,
					 is_18v ? QSPI1_PAD_1V8 : 0);
		if (ret)
			return ret;
		/*
		 * QSPI1 is less auto-probed device, printing this will be
		 * helpful if voltage for some reason differ from current pad
		 * configuration and the initialization of a flash didn't done
		 * correctly.
		 */
		debug("\nHint: QSPI1 pads are configured as %s\n", is_18v ?
		      "1.8V" : "3.3V");

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
	}

	return ret;
}

static int silvaco_qspi_probe(struct udevice *dev)
{
	struct silvaco_qspi_priv *priv = dev_get_priv(dev);
	struct silvaco_qspi_platdata *plat = dev_get_plat(dev);
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
	if (ret < 0 && ret != -ENOENT)
		goto disable_clk_ext;

	if (priv->rst_ctl.dev) {
		ret = reset_deassert(&priv->rst_ctl);
		if (ret < 0)
			goto disable_clk_ext;
	}

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

	return 0;

assert_reset:
	if (priv->rst_ctl.dev) {
		reset_assert(&priv->rst_ctl);
		reset_free(&priv->rst_ctl);
	}
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

static int silvaco_qspi_wait_to_be_not(struct silvaco_qspi_regs *regs, u32 flag)
{
	return wait_for_bit_le32(&regs->stat, flag, false, 1000, false);
}

static int silvaco_qspi_wait_to_be(struct silvaco_qspi_regs *regs, u32 flag)
{
	return wait_for_bit_le32(&regs->stat, flag, true, 1000, false);
}

static int silvaco_qspi_tx_ready(struct silvaco_qspi_regs *regs)
{
	int ret = 0;

	ret = silvaco_qspi_wait_to_be(regs, SILVACO_STAT_TXEMPTY);
	if (ret) {
		debug("%s: Timeout tx not empty\n", __func__);
		return ret;
	}

	ret = silvaco_qspi_wait_to_be_not(regs, SILVACO_STAT_XFERIP);
	if (ret) {
		debug("%s: Timeout xfer ongoing\n", __func__);
		return ret;
	}

	return ret;
}

static void mcom03_silvaco_setup_gpio(struct silvaco_qspi_priv *silvaco)
{
	struct silvaco_qspi_regs *regs = silvaco->regs;

	/*
	 gpo_{set, clr} registers has this bit mapping:
	 gpo[0:3], gpo_oen[4:7], gpo_en[8:11]
	 */
	switch (silvaco->io_lines) {
	case SILVACO_NORMAL:
		/*
		 gpo:
		 Set SISO2 and SISO3 to HIGH.
		 gpo_en:
		 Give access for controling the output on SISO2 and SISO3.
		 */
		writel(GPO_OUTPUT_SISO_2_3 | GPO_ENABLE_SISO_2_3,
		       &regs->gpo_set);
		/*
		 gpo_oen:
		 Give access for enabling/disabling tristate on SISO2 and SISO3.
		 gpo_en:
		 Disable access for controling the output on SISO0 and SISO1.
		 fdpx: Enable full-duplex mode
		 */
		writel(GPO_ENABLE_SISO_0_1 | GPO_TRISTATE_SISO_2_3 |
		       GPO_DISABLE_FULL_DUPLEX, &regs->gpo_clr);
		break;
	case SILVACO_QUAD:
		/*
		 gpo_en: Disabling access to control the output on SISO[0-3]
		 fdpx: Disable full-duplex mode
		 */
		writel(GPO_DISABLE_FULL_DUPLEX, &regs->gpo_set);
		writel(GPO_ENABLE_SISO_0_1 | GPO_ENABLE_SISO_2_3,
		       &regs->gpo_clr);
		break;
	}
}

static int silvaco_qspi_prep_io_lines(struct silvaco_qspi_priv *silvaco,
				      u8 *rxp)
{
	struct silvaco_qspi_regs *regs = silvaco->regs;
	u32 aux = readl(&regs->ctrl_aux);
	int ret;

	ret = silvaco_qspi_tx_ready(regs);
	if (ret)
		return ret;

	switch (silvaco->io_lines) {
	case SILVACO_NORMAL:
		aux &= ~GENMASK(1, 0);
		break;
	case SILVACO_QUAD:
		aux |= GENMASK(1, 0);
		break;
	}

	writel(aux, &regs->ctrl_aux);

	mcom03_silvaco_setup_gpio(silvaco);

	if (rxp)
		writel(readl(&regs->ctrl_aux) & ~SILVACO_AUX_INHDIN,
		       &regs->ctrl_aux);

	return 0;
}

static int silavco_qspi_end_xfer(struct silvaco_qspi_priv *silvaco, u8 *rxp)
{
	struct silvaco_qspi_regs *regs = silvaco->regs;
	int ret;

	/*
	 * Even if there is timeout, we should set INHIDIN and disable cs.
	 */
	ret = silvaco_qspi_tx_ready(regs);

	if (rxp)
		writel(readl(&regs->ctrl_aux) | SILVACO_AUX_INHDIN,
		       &regs->ctrl_aux);

	writel(0, &regs->ss);

	return ret;
}

static int silvaco_qspi_xfer(struct udevice *dev, unsigned int bitlen,
			     const void *dout, void *din, unsigned long flags)
{
	struct silvaco_qspi_priv *priv = dev_get_priv(dev->parent);
	struct dm_spi_slave_plat *slave_plat = dev_get_parent_plat(dev);
	struct silvaco_qspi_regs *regs = priv->regs;
	unsigned int bytes = bitlen / 8;
	const unsigned char *txp = dout;
	unsigned char *rxp = din;
	int ret = 0;
	u32 data;

	debug("%s: bus:%i cs:%i bitlen:%i bytes:%i flags:%lx\n", __func__,
	      dev->parent->seq_, slave_plat->cs, bitlen, bytes, flags);

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

	ret = silvaco_qspi_prep_io_lines(priv, rxp);
	if (ret) {
		debug("%s: error upon preparing io lines\n", __func__);
		flags |= SPI_XFER_END;
		goto done;
	}

	if (flags & SPI_XFER_BEGIN)
		writel(BIT(slave_plat->cs), &regs->ss);

	while (bytes--) {
		if (txp)
			data = *txp++;
		else
			data = SILVACO_DUMMY_BYTES;

		ret = silvaco_qspi_wait_to_be_not(regs, SILVACO_STAT_TXFULL);
		if (ret) {
			debug("%s: Transmit timed out\n", __func__);
			flags |= SPI_XFER_END;
			goto done;
		}

		if (txp)
			debug("%s: tx:%x\n", __func__, data);

		writel(data, &regs->tx_data);

		if (rxp) {
			ret = silvaco_qspi_wait_to_be_not(regs,
							  SILVACO_STAT_RXEMPTY);
			if (ret) {
				debug("%s: Receive timed out\n", __func__);
				flags |= SPI_XFER_END;
				goto done;
			}

			data = readl(&regs->rx_data);
			*rxp++ = data & 0xff;

			debug("%s: rx:%x\n", __func__, data);
		}
	}
done:
	if (flags & SPI_XFER_END)
		ret = silavco_qspi_end_xfer(priv, rxp);

	return ret;
}

static int silvaco_qspi_set_speed(struct udevice *dev, uint speed)
{
	return 0;
}

static int silvaco_qspi_set_mode(struct udevice *dev, uint mode)
{
	int ret = 0;

	if (mode & (SPI_TX_DUAL | SPI_RX_DUAL)) {
		printf("Silvaco driver does not support dual transmit\n");
		ret = -EINVAL;
	} else if (mode & (SPI_TX_OCTAL | SPI_RX_OCTAL)) {
		printf("Silvaco controller does not support octal transmit\n");
		ret = -EINVAL;
	}

	return ret;
}

int silvaco_exec_op(struct spi_slave *slave, const struct spi_mem_op *op)
{
	struct udevice *dev = slave->dev;
	struct silvaco_qspi_priv *priv = dev_get_priv(dev->parent);
	u8 opcode = op->cmd.opcode;
	unsigned long flags = SPI_XFER_BEGIN;
	const void *txp = NULL;
	void *rxp = NULL;
	unsigned int pos = 0;
	int op_len, i;
	int ret;

	if (!op->addr.nbytes && !op->dummy.nbytes && !op->data.nbytes)
		flags |= SPI_XFER_END;

	priv->io_lines = op->cmd.buswidth;

	/* send the opcode */
	ret = silvaco_qspi_xfer(dev, 8, (void *)&opcode, NULL, flags);
	if (ret < 0) {
		printf("Failed to xfer opcode %xh\n", opcode);
		return ret;
	}

	op_len = op->addr.nbytes + op->dummy.nbytes;
	u8 op_buf[op_len];

	/* send the addr + dummy */
	if (op->addr.nbytes) {
		/* fill address */
		for (i = 0; i < op->addr.nbytes; i++)
			op_buf[pos + i] = op->addr.val >>
				(8 * (op->addr.nbytes - i - 1));

		pos += op->addr.nbytes;

		/* fill dummy */
		if (op->dummy.nbytes)
			memset(op_buf + pos, SILVACO_DUMMY_BYTES,
			       op->dummy.nbytes);

		/*
		 * Some commands doesn't require data(like sector erase), so we
		 * need to end the xfer after sending the addr + dummy.
		 */
		if (!op->data.nbytes)
			flags |= SPI_XFER_END;

		priv->io_lines = op->addr.buswidth;

		ret = silvaco_qspi_xfer(dev, op_len * 8, op_buf, NULL, flags);
		if (ret < 0) {
			debug("Failed to xfer addr + dummy\n");
			return ret;
		}
	}

	/* send/received the data */
	if (op->data.nbytes) {
		if (op->data.dir == SPI_MEM_DATA_IN)
			rxp = op->data.buf.in;
		else
			txp = op->data.buf.out;

		priv->io_lines = op->data.buswidth;

		ret = silvaco_qspi_xfer(dev, op->data.nbytes * 8, txp, rxp,
					SPI_XFER_END);
		if (ret) {
			debug("Failed to xfer data\n");
			return ret;
		}
	}

	return 0;
}

const struct spi_controller_mem_ops silvaco_mem_ops = {
	.exec_op	= silvaco_exec_op,
};

static const struct dm_spi_ops silvaco_qspi_ops = {
	.claim_bus	= silvaco_qspi_claim_bus,
	.release_bus	= silvaco_qspi_release_bus,
	.set_speed	= silvaco_qspi_set_speed,
	.set_mode	= silvaco_qspi_set_mode,
	.mem_ops	= &silvaco_mem_ops,
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
	.of_to_plat = silvaco_qspi_ofdata_to_platdata,
	.plat_auto = sizeof(struct silvaco_qspi_platdata),
	.priv_auto = sizeof(struct silvaco_qspi_priv),
	.probe	= silvaco_qspi_probe,
	.remove	= silvaco_qspi_remove,
};
