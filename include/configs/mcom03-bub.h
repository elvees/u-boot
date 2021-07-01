/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2021 RnD Center "ELVEES", JSC
 *
 * Configuration settings for the MCom-03 Bring-Up board
 */

#ifndef __MCOM03_BUB_H
#define __MCOM03_BUB_H

#include <linux/sizes.h>

/* Specify available DDR memory.
 * These macros are also used for specifying DDR size available for Linux.
 */
#define CONFIG_SYS_SDRAM_BASE		0xC0000000
#define PHYS_SDRAM_0			CONFIG_SYS_SDRAM_BASE
#define PHYS_SDRAM_0_SIZE		SZ_1G
#define CONFIG_SYS_MALLOC_LEN		SZ_8M

#define CONFIG_SYS_INIT_SP_ADDR		0xC0400000

#define CONFIG_SYS_LOAD_ADDR		0xC2000000 /* default load address */

/* standalone support */
#define CONFIG_STANDALONE_LOAD_ADDR	0xC2000000

/* Default environment */
#define CONFIG_BOOTFILE			"Image"
#define CONFIG_LOADADDR			CONFIG_SYS_LOAD_ADDR

#define CONFIG_BOOTCOMMAND \
	"booti ${loadaddr} - ${fdtcontroladdr}"

#endif
