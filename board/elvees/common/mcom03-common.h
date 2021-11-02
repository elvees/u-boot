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

void lsperiph1_v18_pad_cfg(void);
void i2c_pad_cfg(int i2c_num);
void board_pads_cfg(void);
void nand_pad_cfg(void);

#endif
