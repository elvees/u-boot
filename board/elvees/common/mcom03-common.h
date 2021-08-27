/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 */

#ifndef __MCOM03_COMMON
#define __MCOM03_COMMON

#include <linux/bitops.h>

#define GPIO1_PORTA_PAD_CTR(x)		(0x17e0020UL + (x) * 0x4)
#define GPIO1_PORTD_PAD_CTR(x)		(0x17e0080UL + (x) * 0x4)
#define GPIO_PAD_CTR_EN			BIT(12)

#define LSP1_GPIO_SWPORTA_CTL		0x1780008
#define LSP0_GPIO_SWPORTD_CTL		0x161002c

void i2c_pad_cfg(int i2c_num);
void board_pads_cfg(void);

#endif
