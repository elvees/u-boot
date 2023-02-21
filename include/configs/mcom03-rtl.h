/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019 RnD Center "ELVEES", JSC
 *
 * Configuration settings for the MCom-03 platform
 */

#ifndef __MCOM_03_RTL_H
#define __MCOM_03_RTL_H

#include <linux/sizes.h>

#define CONFIG_SYS_SDRAM_BASE		0xC0000000
#define PHYS_SDRAM_0			CONFIG_SYS_SDRAM_BASE
#define PHYS_SDRAM_0_SIZE		SZ_1M

/* standalone support */
#define CONFIG_STANDALONE_LOAD_ADDR	0xC2000000

/* write a magic number to finish */
#define CONFIG_BOOTCOMMAND		"mw 0xC0005000 0x12345678"

#endif
