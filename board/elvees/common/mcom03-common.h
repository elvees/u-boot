/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 */

#ifndef __MCOM03_COMMON
#define __MCOM03_COMMON

#include <linux/bitops.h>

#define GPIO1_PORTA_PAD_CTR(x)		(0x17e0020UL + (x) * 0x4)
#define GPIO1_PORTC_PAD_CTR(x)		(0x17e0060UL + (x) * 0x4)
#define GPIO1_PORTD_PAD_CTR(x)		(0x17e0080UL + (x) * 0x4)
#define GPIO_PAD_CTR_EN			BIT(12)

#define LSP1_URB_GPIO1_PAD_CTR		0x17e0020UL
#define LSP1_URB_GPIO1_PAD_CTR_SUS	BIT(0)
#define LSP1_URB_GPIO1_PAD_CTR_PU	BIT(1)
#define LSP1_URB_GPIO1_PAD_CTR_PD	BIT(2)
#define LSP1_URB_GPIO1_PAD_CTR_SL	GENMASK(4, 3)
#define LSP1_URB_GPIO1_PAD_CTR_CTL	GENMASK(10, 5)
#define LSP1_URB_GPIO1_PAD_CTR_E	BIT(12)
#define LSP1_URB_GPIO1_PAD_CTR_CLE	BIT(13)
#define LSP1_URB_GPIO1_PAD_CTR_OD	BIT(14)

#define LSP1_URB_GPIO1_PAD_CTR_ADDR(port, num) (LSP1_URB_GPIO1_PAD_CTR + \
						((port) * 8 + (num)) * 4)

#define LSP1_URB_GPIO1_V18		0x17e00a0UL
#define LSP1_URB_GPIO1_V18_V18		BIT(0)

#define LSP0_GPIO_SWPORTD_CTL		0x161002c

#define LSP1_GPIO_SWPORTA_DR		0x1780000
#define LSP1_GPIO_SWPORTA_DDR		(0x1780000 + 0x04)
#define LSP1_GPIO_SWPORTA_CTL		(0x1780000 + 0x08)
#define LSP1_GPIO_SWPORTC_DR		(0x1780000 + 0x18)
#define LSP1_GPIO_SWPORTC_DDR		(0x1780000 + 0x1C)
#define LSP1_GPIO_SWPORTC_CTL		(0x1780000 + 0x20)
#define LSP1_GPIO_SWPORTD_DR		(0x1780000 + 0x24)
#define LSP1_GPIO_SWPORTD_DDR		(0x1780000 + 0x28)
#define LSP1_GPIO_SWPORTD_CTL		(0x1780000 + 0x2C)

#define GPIO_PORTA			0
#define GPIO_PORTB			1
#define GPIO_PORTC			2
#define GPIO_PORTD			3

#define MFBSP1_DIR			0x1f0b1308
#define MFBSP1_DR			0x1f0b130c

#define HSP_URB_XIP_EN_REQ		0x10400010
#define HSP_URB_XIP_EN_OUT		0x10400014

#define HSP_URB_EMAC0_TX_PADCFG		0x10400148
#define HSP_URB_EMAC0_TXC_PADCFG	0x1040014c
#define HSP_URB_EMAC1_TX_PADCFG		0x10400168
#define HSP_URB_EMAC1_TXC_PADCFG	0x1040016c

#define SERVICE_URB_XIP_EN_REQ		0x1f002004
#define SERVICE_URB_XIP_EN_OUT		0x1f002008

#define QSPI_XIP_EN			BIT(0)

void lsperiph1_v18_pad_cfg(void);
void i2c_pad_cfg(int i2c_num);
void nand_pad_cfg(void);
void board_pads_cfg(void);
void nand_pad_cfg(void);
void pad_set_ctl(unsigned long reg, u32 value);
int do_factory_settings(void);

#endif
