// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2017-2020 RnD Center "ELVEES", JSC
 *
 * Arasan Gigabit Ethernet MAC driver
 */

#include <common.h>
#include <asm/gpio.h>
#include <dm.h>
#include <linux/io.h>
#include <memalign.h>
#include <miiphy.h>
#include <net.h>
#include <phy.h>
#include <reset.h>
#include <linux/delay.h>
#include <cpu_func.h>

DECLARE_GLOBAL_DATA_PTR;

/* DMA Register Set */
#define DMA_CONFIG				0x0000
#define DMA_CTRL				0x0004
#define DMA_STATUS_AND_IRQ			0x0008
#define DMA_INT_ENABLE				0x000C
#define DMA_TX_AUTO_POLL_COUNTER		0x0010
#define DMA_TX_POLL_DEMAND			0x0014
#define DMA_RX_POLL_DEMAND			0x0018
#define DMA_TX_BASE_ADDR			0x001C
#define DMA_RX_BASE_ADDR			0x0020
#define DMA_MISSED_FRAME_COUNTER		0x0024
#define DMA_STOP_FLUSH_COUNTER			0x0028
#define DMA_RX_INT_MITIGATION_CTRL		0x002C
#define DMA_CURRENT_TX_DESC_PTR			0x0030
#define DMA_CURRENT_TX_BUFFER_PTR		0x0034
#define DMA_CURRENT_RX_DESC_PTR			0x0038
#define DMA_CURRENT_RX_BUFFER_PTR		0x003C

/* DMA Configuration Register */
#define DMA_CONFIG_DESC_SKIP_LENGTH(v)		((v) << 8)
#define DMA_CONFIG_BURST_LENGTH(v)		((v) << 1)
#define DMA_CONFIG_SOFT_RESET			BIT(0)

/* DMA Control Register */
#define DMA_CTRL_START_RX_DMA			BIT(1)
#define DMA_CTRL_START_TX_DMA			BIT(0)

/* MAC Register Set */
#define MAC_GLOBAL_CTRL				0x0100
#define MAC_TX_CTRL				0x0104
#define MAC_RX_CTRL				0x0108
#define MAC_MAX_FRAME_SIZE			0x010C
#define MAC_TX_JABBER_SIZE			0x0110
#define MAC_RX_JABBER_SIZE			0x0114
#define MAC_ADDR_CTRL				0x0118
#define MAC_MDIO_CLK_DIV_CTRL			0x011C
#define MAC_ADDR1_HIGH				0x0120
#define MAC_ADDR1_MED				0x0124
#define MAC_ADDR1_LOW				0x0128
#define MAC_ADDR2_HIGH				0x012C
#define MAC_ADDR2_MED				0x0130
#define MAC_ADDR2_LOW				0x0134
#define MAC_ADDR3_HIGH				0x0138
#define MAC_ADDR3_MED				0x013C
#define MAC_ADDR3_LOW				0x0140
#define MAC_ADDR4_HIGH				0x0144
#define MAC_ADDR4_MED				0x0148
#define MAC_ADDR4_LOW				0x014C
#define MAC_HASH_TABLE1				0x0150
#define MAC_HASH_TABLE2				0x0154
#define MAC_HASH_TABLE3				0x0158
#define MAC_HASH_TABLE4				0x015C
#define MAC_MDIO_CTRL				0x01A0
#define MAC_MDIO_DATA				0x01A4
#define MAC_TX_FIFO_ALMOST_FULL_THRESHOLD	0x01C0
#define MAC_TX_PACKET_START_THRESHOLD		0x01C4
#define MAC_RX_PACKET_START_THRESHOLD		0x01C8
#define MAC_TX_FIFO_ALMOST_EMPTY_THRESHOLD	0x01CC

/* MAC Global Control Register */
#define MAC_GLOBAL_CTRL_FULL_DUPLEX_MODE	BIT(2)
#define MAC_GLOBAL_CTRL_SPEED(v)		((v) << 0)

#define MAC_GLOBAL_CTRL_SPEED_10_MBPS		MAC_GLOBAL_CTRL_SPEED(0)
#define MAC_GLOBAL_CTRL_SPEED_100_MBPS		MAC_GLOBAL_CTRL_SPEED(1)
#define MAC_GLOBAL_CTRL_SPEED_1000_MBPS		MAC_GLOBAL_CTRL_SPEED(2)

/* MAC Transmit Control Register */
#define MAC_TX_CTRL_TX_AUTO_RETRY		BIT(3)
#define MAC_TX_CTRL_TX_ENABLE			BIT(0)

/* MAC Receive Control Register */
#define MAC_RX_CTRL_STORE_AND_FORWARD		BIT(3)
#define MAC_RX_CTRL_RX_ENABLE			BIT(0)

/* MAC Address Control Register */
#define MAC_ADDR_CTRL_MAC_ADDR4_ENABLE		BIT(3)
#define MAC_ADDR_CTRL_MAC_ADDR3_ENABLE		BIT(2)
#define MAC_ADDR_CTRL_MAC_ADDR2_ENABLE		BIT(1)
#define MAC_ADDR_CTRL_MAC_ADDR1_ENABLE		BIT(0)

/* MAC MDIO Control Register */
#define MAC_MDIO_CTRL_START_MDIO_TRANSACTION	BIT(15)
#define MAC_MDIO_CTRL_MDIO_READ			BIT(10)
#define MAC_MDIO_CTRL_REG_ADDR(v)		((v) << 5)
#define MAC_MDIO_CTRL_PHY_ADDR(v)		((v) << 0)

struct arasan_gemac_dma_desc {
	u32 status;
#define DMA_DESC_STATUS_OWN			BIT(31)
#define DMA_DESC_STATUS_FIRST_DESC		BIT(30)
#define DMA_DESC_STATUS_LAST_DESC		BIT(29)
	u32 ctrl;
#define DMA_DESC_CTRL_LAST_SEGMENT		BIT(30)
#define DMA_DESC_CTRL_FIRST_SEGMENT		BIT(29)
#define DMA_DESC_CTRL_END_OF_THE_RING		BIT(26)
#define DMA_DESC_CTRL_BUFFER2_SIZE(v)		((v) << 12)
#define DMA_DESC_CTRL_BUFFER1_SIZE(v)		((v) << 0)
	u32 addr1;
	u32 addr2;
} __aligned(ARCH_DMA_MINALIGN) __packed;

#define DESC_SKIP_LENGTH	(ARCH_DMA_MINALIGN / 4 - 4)

#define TX_FRAME_FIFO_DEPTH	512

#define TX_DESC_NUMBER		16
#define RX_DESC_NUMBER		16

#define HSP_EMAC_PADCFG(i)	(0x10400144UL + 0x20 * (i))
#define HSP_EMAC_PADS_1V8_EN	0x10400140UL

struct arasan_gemac_priv {
	void *base;
	struct reset_ctl rst_ctl;
	struct mii_dev *bus;
	struct phy_device *phydev;
	int phy_addr;
	phy_interface_t phy_interface;
	struct gpio_desc phy_reset;
	struct gpio_desc phy_txclk;
	u8 *tx_buffer;
	u8 *rx_buffer;
	u8 *tx_buffer_ptr;
	u8 *rx_buffer_ptr;
	struct arasan_gemac_dma_desc *tx_desc_ptr;
	struct arasan_gemac_dma_desc *rx_desc_ptr;
	struct arasan_gemac_dma_desc *tx_desc_ring;
	struct arasan_gemac_dma_desc *rx_desc_ring;
	u32 ctrl_id;
};

static inline void arasan_gemac_flush_dcache(uintptr_t start, size_t size)
{
	flush_dcache_range(rounddown(start, ARCH_DMA_MINALIGN),
			   roundup(start + size, ARCH_DMA_MINALIGN));
}

static inline void arasan_gemac_invalidate_dcache(uintptr_t start, size_t size)
{
	invalidate_dcache_range(rounddown(start, ARCH_DMA_MINALIGN),
				roundup(start + size, ARCH_DMA_MINALIGN));
}

static int arasan_gemac_buffers_alloc(struct udevice *dev)
{
	struct arasan_gemac_priv *priv = dev_get_priv(dev);

	priv->tx_desc_ring = malloc_cache_aligned(TX_DESC_NUMBER *
						  sizeof(*priv->tx_desc_ring));
	if (!priv->tx_desc_ring)
		return -ENOMEM;

	priv->rx_desc_ring = malloc_cache_aligned(RX_DESC_NUMBER *
						  sizeof(*priv->rx_desc_ring));
	if (!priv->rx_desc_ring)
		return -ENOMEM;

	priv->tx_buffer = malloc_cache_aligned(TX_DESC_NUMBER * PKTSIZE_ALIGN);
	if (!priv->tx_buffer)
		return -ENOMEM;

	priv->rx_buffer = malloc_cache_aligned(RX_DESC_NUMBER * PKTSIZE_ALIGN);
	if (!priv->rx_buffer)
		return -ENOMEM;

	return 0;
}

static void arasan_gemac_buffers_free(struct udevice *dev)
{
	struct arasan_gemac_priv *priv = dev_get_priv(dev);

	free(priv->tx_desc_ring);
	free(priv->rx_desc_ring);
	free(priv->tx_buffer);
	free(priv->rx_buffer);
}

static void arasan_gemac_tx_desc_ring_init(struct udevice *dev)
{
	struct arasan_gemac_priv *priv = dev_get_priv(dev);
	struct arasan_gemac_dma_desc *desc_ring = priv->tx_desc_ring;
	uintptr_t buffer_addr = (uintptr_t)priv->tx_buffer;
	int desc;

	for (desc = 0; desc < TX_DESC_NUMBER; ++desc) {
		desc_ring[desc].status = 0;
		desc_ring[desc].ctrl = 0;
		desc_ring[desc].addr1 = buffer_addr;
		desc_ring[desc].addr2 = 0;

		buffer_addr += PKTSIZE_ALIGN;
	}

	desc_ring[TX_DESC_NUMBER - 1].ctrl |= DMA_DESC_CTRL_END_OF_THE_RING;

	priv->tx_desc_ptr = desc_ring;
	priv->tx_buffer_ptr = priv->tx_buffer;

	arasan_gemac_flush_dcache((uintptr_t)desc_ring,
				  TX_DESC_NUMBER * sizeof(*desc_ring));

	writel((uintptr_t)desc_ring, priv->base + DMA_TX_BASE_ADDR);
}

static void arasan_gemac_rx_desc_ring_init(struct udevice *dev)
{
	struct arasan_gemac_priv *priv = dev_get_priv(dev);
	struct arasan_gemac_dma_desc *desc_ring = priv->rx_desc_ring;
	uintptr_t buffer_addr = (uintptr_t)priv->rx_buffer;
	int desc;

	for (desc = 0; desc < RX_DESC_NUMBER; ++desc) {
		desc_ring[desc].status = DMA_DESC_STATUS_OWN;
		desc_ring[desc].ctrl =
			DMA_DESC_CTRL_BUFFER1_SIZE(PKTSIZE_ALIGN);
		desc_ring[desc].addr1 = buffer_addr;
		desc_ring[desc].addr2 = 0;

		buffer_addr += PKTSIZE_ALIGN;
	}

	desc_ring[RX_DESC_NUMBER - 1].ctrl |= DMA_DESC_CTRL_END_OF_THE_RING;

	priv->rx_desc_ptr = desc_ring;
	priv->rx_buffer_ptr = priv->rx_buffer;

	arasan_gemac_flush_dcache((uintptr_t)desc_ring,
				  RX_DESC_NUMBER * sizeof(*desc_ring));

	arasan_gemac_flush_dcache((uintptr_t)priv->rx_buffer,
				  RX_DESC_NUMBER * PKTSIZE_ALIGN);

	writel((uintptr_t)desc_ring, priv->base + DMA_RX_BASE_ADDR);
}

static void arasan_gemac_adjust_link(struct udevice *dev)
{
	struct arasan_gemac_priv *priv = dev_get_priv(dev);
	struct phy_device *phydev = priv->phydev;
	u32 mac_global_ctrl = 0;

	if (phydev->duplex == DUPLEX_FULL)
		mac_global_ctrl |= MAC_GLOBAL_CTRL_FULL_DUPLEX_MODE;

	/* The Transmit Packet Start Threshold values are based on the
	 * interface speed as suggested by the Arasan GEMAC Functional
	 * Specification. */
	switch (phydev->speed) {
	case SPEED_10:
		mac_global_ctrl |= MAC_GLOBAL_CTRL_SPEED_10_MBPS;
		writel(64, priv->base + MAC_TX_PACKET_START_THRESHOLD);
		break;
	case SPEED_100:
		mac_global_ctrl |= MAC_GLOBAL_CTRL_SPEED_100_MBPS;
		writel(128, priv->base + MAC_TX_PACKET_START_THRESHOLD);
		break;
	case SPEED_1000:
		mac_global_ctrl |= MAC_GLOBAL_CTRL_SPEED_1000_MBPS;
		writel(1024, priv->base + MAC_TX_PACKET_START_THRESHOLD);
		break;
	}

	writel(mac_global_ctrl, priv->base + MAC_GLOBAL_CTRL);

#ifdef CONFIG_DM_GPIO
	if (dm_gpio_is_valid(&priv->phy_txclk))
		dm_gpio_set_value(&priv->phy_txclk,
				  phydev->speed == SPEED_1000);
#endif
}

static int arasan_gemac_start(struct udevice *dev)
{
	struct arasan_gemac_priv *priv = dev_get_priv(dev);
	int ret;

	ret = phy_startup(priv->phydev);
	if (ret != 0)
		return ret;

	arasan_gemac_adjust_link(dev);

	/* The Transmit FIFO Almost Full Threshold value is set as required by
	 * the Arasan GEMAC Functional Specification. */
	writel(TX_FRAME_FIFO_DEPTH - 8,
	       priv->base + MAC_TX_FIFO_ALMOST_FULL_THRESHOLD);

	writel(DMA_CONFIG_SOFT_RESET, priv->base + DMA_CONFIG);

	writel(DMA_CONFIG_DESC_SKIP_LENGTH(DESC_SKIP_LENGTH) |
	       DMA_CONFIG_BURST_LENGTH(4),
	       priv->base + DMA_CONFIG);

	arasan_gemac_tx_desc_ring_init(dev);
	arasan_gemac_rx_desc_ring_init(dev);

	writel(MAC_TX_CTRL_TX_AUTO_RETRY | MAC_TX_CTRL_TX_ENABLE,
	       priv->base + MAC_TX_CTRL);

	writel(MAC_RX_CTRL_STORE_AND_FORWARD | MAC_RX_CTRL_RX_ENABLE,
	       priv->base + MAC_RX_CTRL);

	writel(DMA_CTRL_START_RX_DMA | DMA_CTRL_START_TX_DMA,
	       priv->base + DMA_CTRL);

	return 0;
}

static int arasan_gemac_send(struct udevice *dev, void *packet, int length)
{
	struct arasan_gemac_priv *priv = dev_get_priv(dev);
	struct arasan_gemac_dma_desc *desc = priv->tx_desc_ptr;

	if (!packet || length < 0 || length > PKTSIZE)
		return -EINVAL;

	arasan_gemac_invalidate_dcache((uintptr_t)desc, sizeof(*desc));

	/* At present, the uclass driver does not check return codes from this
	 * function, and the function should wait for at least one descriptor
	 * to be released by DMA. The waiting is omitted since there are always
	 * enough number of descriptors in the ring to hold all transmit
	 * packets. */
	if (desc->status & DMA_DESC_STATUS_OWN)
		return -EAGAIN;

	memcpy((void *)priv->tx_buffer_ptr, packet, length);

	arasan_gemac_flush_dcache((uintptr_t)priv->tx_buffer_ptr, length);

	desc->ctrl &= DMA_DESC_CTRL_END_OF_THE_RING;
	desc->ctrl |= DMA_DESC_CTRL_LAST_SEGMENT |
		      DMA_DESC_CTRL_FIRST_SEGMENT |
		      DMA_DESC_CTRL_BUFFER1_SIZE(length);

	desc->status = DMA_DESC_STATUS_OWN;

	arasan_gemac_flush_dcache((uintptr_t)desc, sizeof(*desc));

	writel(1, priv->base + DMA_TX_POLL_DEMAND);

	if (++priv->tx_desc_ptr == priv->tx_desc_ring + TX_DESC_NUMBER)
		priv->tx_desc_ptr = priv->tx_desc_ring;

	priv->tx_buffer_ptr += PKTSIZE_ALIGN;

	if ((uintptr_t)priv->tx_buffer_ptr ==
	    (uintptr_t)priv->tx_buffer + TX_DESC_NUMBER * PKTSIZE_ALIGN)
		priv->tx_buffer_ptr = priv->tx_buffer;

	return 0;
}

static int arasan_gemac_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct arasan_gemac_priv *priv = dev_get_priv(dev);
	struct arasan_gemac_dma_desc *desc = priv->rx_desc_ptr;
	int length;

	arasan_gemac_invalidate_dcache((uintptr_t)desc, sizeof(*desc));

	if (desc->status & DMA_DESC_STATUS_OWN)
		return -EAGAIN;

	length = desc->status & 0x3fff;

	arasan_gemac_invalidate_dcache((uintptr_t)priv->rx_buffer_ptr, length);

	*packetp = (uchar *)priv->rx_buffer_ptr;

	return length;
}

static int arasan_gemac_free_pkt(struct udevice *dev, uchar *packet,
				 int length)
{
	struct arasan_gemac_priv *priv = dev_get_priv(dev);
	struct arasan_gemac_dma_desc *desc = priv->rx_desc_ptr;

	desc->status = DMA_DESC_STATUS_OWN;

	arasan_gemac_flush_dcache((uintptr_t)desc, sizeof(*desc));

	writel(1, priv->base + DMA_RX_POLL_DEMAND);

	if (++priv->rx_desc_ptr == priv->rx_desc_ring + RX_DESC_NUMBER)
		priv->rx_desc_ptr = priv->rx_desc_ring;

	priv->rx_buffer_ptr += PKTSIZE_ALIGN;

	if ((uintptr_t)priv->rx_buffer_ptr ==
	    (uintptr_t)priv->rx_buffer + RX_DESC_NUMBER * PKTSIZE_ALIGN)
		priv->rx_buffer_ptr = priv->rx_buffer;

	return 0;
}

static void arasan_gemac_stop(struct udevice *dev)
{
	struct arasan_gemac_priv *priv = dev_get_priv(dev);

	writel(0, priv->base + DMA_CTRL);
	writel(0, priv->base + MAC_TX_CTRL);
	writel(0, priv->base + MAC_RX_CTRL);

	phy_shutdown(priv->phydev);
}

static int arasan_gemac_write_hwaddr(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_plat(dev);
	struct arasan_gemac_priv *priv = dev_get_priv(dev);
	unsigned char *enetaddr = pdata->enetaddr;

	writel(enetaddr[5] << 8 | enetaddr[4], priv->base + MAC_ADDR1_LOW);
	writel(enetaddr[3] << 8 | enetaddr[2], priv->base + MAC_ADDR1_MED);
	writel(enetaddr[1] << 8 | enetaddr[0], priv->base + MAC_ADDR1_HIGH);

	writel(MAC_ADDR_CTRL_MAC_ADDR1_ENABLE, priv->base + MAC_ADDR_CTRL);

	return 0;
}

const struct eth_ops arasan_gemac_ops = {
	.start		= arasan_gemac_start,
	.send		= arasan_gemac_send,
	.recv		= arasan_gemac_recv,
	.free_pkt	= arasan_gemac_free_pkt,
	.stop		= arasan_gemac_stop,
	.write_hwaddr	= arasan_gemac_write_hwaddr,
};

static int arasan_gemac_mdio_read(struct mii_dev *bus, int addr, int devad,
				  int reg)
{
	struct arasan_gemac_priv *priv = bus->priv;

	writel(MAC_MDIO_CTRL_PHY_ADDR(addr) |
	       MAC_MDIO_CTRL_REG_ADDR(reg) |
	       MAC_MDIO_CTRL_MDIO_READ |
	       MAC_MDIO_CTRL_START_MDIO_TRANSACTION,
	       priv->base + MAC_MDIO_CTRL);

	while (readl(priv->base + MAC_MDIO_CTRL) &
		MAC_MDIO_CTRL_START_MDIO_TRANSACTION)
		;

	return readl(priv->base + MAC_MDIO_DATA);
}

static int arasan_gemac_mdio_write(struct mii_dev *bus, int addr, int devad,
				   int reg, u16 val)
{
	struct arasan_gemac_priv *priv = bus->priv;

	writel(val, priv->base + MAC_MDIO_DATA);
	writel(MAC_MDIO_CTRL_PHY_ADDR(addr) |
	       MAC_MDIO_CTRL_REG_ADDR(reg) |
	       MAC_MDIO_CTRL_START_MDIO_TRANSACTION,
	       priv->base + MAC_MDIO_CTRL);

	while (readl(priv->base + MAC_MDIO_CTRL) &
		MAC_MDIO_CTRL_START_MDIO_TRANSACTION)
		;

	return 0;
}

static int arasan_gemac_mdio_reset(struct mii_dev *bus)
{
#ifdef CONFIG_DM_GPIO
	struct arasan_gemac_priv *priv = bus->priv;

	if (dm_gpio_is_valid(&priv->phy_reset)) {
		dm_gpio_set_value(&priv->phy_reset, 1);
		udelay(1000);
		dm_gpio_set_value(&priv->phy_reset, 0);

		/* PHY on MCom-03 BuB requires delay after reset cycle. */
		mdelay(100);
	}
#endif
	return 0;
}

static int arasan_gemac_mdio_init(struct udevice *dev)
{
	struct arasan_gemac_priv *priv = dev_get_priv(dev);

	priv->bus = mdio_alloc();
	if (!priv->bus)
		return -ENOMEM;

	strcpy(priv->bus->name, dev->name);
	priv->bus->priv = priv;
	priv->bus->read = arasan_gemac_mdio_read;
	priv->bus->write = arasan_gemac_mdio_write;
	priv->bus->reset = arasan_gemac_mdio_reset;

	mdio_register(priv->bus);

	return 0;
}

static int arasan_gemac_phy_init(struct udevice *dev)
{
	struct arasan_gemac_priv *priv = dev_get_priv(dev);

	priv->phydev = phy_connect(priv->bus, priv->phy_addr, dev,
				   priv->phy_interface);
	if (!priv->phydev)
		return -ENODEV;

	phy_config(priv->phydev);

	return 0;
}

static int arasan_gemac_probe(struct udevice *dev)
{
	struct arasan_gemac_priv *priv = dev_get_priv(dev);
	int phy_handle, ret;

	priv->base = (void *)devfdt_get_addr(dev);

	priv->phy_interface = dev_read_phy_mode(dev);
	if (priv->phy_interface == PHY_INTERFACE_MODE_NA)
		return -EINVAL;

	phy_handle = fdtdec_lookup_phandle(gd->fdt_blob, dev_of_offset(dev),
					   "phy-handle");
	if (phy_handle < 0)
		return -EINVAL;

	priv->phy_addr = fdtdec_get_int(gd->fdt_blob, phy_handle, "reg", -1);
	if (priv->phy_addr < 0)
		return -EINVAL;

#ifdef CONFIG_DM_GPIO
	ret = gpio_request_by_name(dev, "phy-reset-gpios", 0,
				   &priv->phy_reset, GPIOD_IS_OUT);
	if (ret != 0 && ret != -ENOENT)
		return ret;

	ret = gpio_request_by_name(dev, "txclk-125en-gpios", 0,
				   &priv->phy_txclk, GPIOD_IS_OUT);
	if (ret != 0 && ret != -ENOENT)
		return ret;
#endif

	ret = reset_get_by_index(dev, 0, &priv->rst_ctl);
	if (ret != 0 && ret != -ENOTSUPP)
		return ret;

	ret = reset_deassert(&priv->rst_ctl);
	if (ret != 0)
		return ret;

	/* TODO: Is it really required ? */
	mdelay(100);

	ret = arasan_gemac_mdio_init(dev);
	if (ret != 0)
		goto error_assert_reset;

	ret = dev_read_u32(dev, "elvees,ctrl-id", &priv->ctrl_id);
	if (ret < 0)
		goto error_assert_reset;

	/* TODO: Pads should be configured using pinctrl driver */
	writel(dev_read_bool(dev, "elvees,pads-1v8-en"), HSP_EMAC_PADS_1V8_EN);
	writel(1, HSP_EMAC_PADCFG(priv->ctrl_id));

	ret = arasan_gemac_phy_init(dev);
	if (ret != 0)
		goto error_phy_init;

	ret = arasan_gemac_buffers_alloc(dev);
	if (ret != 0)
		goto error_buffers_alloc;

	/* TODO: Remove this extra PHY reset cycle */
	if (IS_ENABLED(CONFIG_TARGET_ELVMC03SMARC))
		arasan_gemac_mdio_reset(priv->bus);

	return 0;

error_buffers_alloc:
	arasan_gemac_buffers_free(dev);

	free(priv->phydev);

error_phy_init:
	mdio_unregister(priv->bus);
	mdio_free(priv->bus);

error_assert_reset:
	reset_assert(&priv->rst_ctl);
	return ret;
}

int arasan_gemac_remove(struct udevice *dev)
{
	struct arasan_gemac_priv *priv = dev_get_priv(dev);

	arasan_gemac_buffers_free(dev);

	free(priv->phydev);

	mdio_unregister(priv->bus);
	mdio_free(priv->bus);

	return reset_assert(&priv->rst_ctl);
}

static const struct udevice_id arasan_gemac_match_table[] = {
	{ .compatible = "elvees,arasan-gemac" },
	{ },
};

U_BOOT_DRIVER(arasan_gemac_drv) = {
	.name = "arasan_gemac",
	.id = UCLASS_ETH,
	.of_match = arasan_gemac_match_table,
	.probe = arasan_gemac_probe,
	.remove = arasan_gemac_remove,
	.priv_auto = sizeof(struct arasan_gemac_priv),
	.plat_auto = sizeof(struct eth_pdata),
	.ops = &arasan_gemac_ops,
};
