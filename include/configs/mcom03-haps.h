/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2019-2023 RnD Center "ELVEES", JSC
 *
 * Configuration settings for the MCom-03 platform
 */

#ifndef __MCOM_03_HAPS_H
#define __MCOM_03_HAPS_H

#include <linux/sizes.h>

/* Specify available DDR memory.
 * These macros are also used for specifying DDR size available for Linux.
 */
#define CFG_SYS_SDRAM_BASE		0xC0000000
#define PHYS_SDRAM_0			CFG_SYS_SDRAM_BASE
#define PHYS_SDRAM_0_SIZE		SZ_1G
#define PHYS_SDRAM_1			0x80000000
#define PHYS_SDRAM_1_SIZE		SZ_1G

/* Default environment */
#define CONFIG_BOOTFILE			"Image"

#endif
