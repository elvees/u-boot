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
#define CONFIG_LOADADDR			CONFIG_SYS_LOAD_ADDR

#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(MMC, mmc, 1) \
	func(PXE, pxe, na)

#include <config_distro_bootcmd.h>

#define CONFIG_EXTRA_ENV_SETTINGS \
	"disable_giga=0x1\0" \
	"kernel_addr_r=0xc2000000\0" \
	"scriptaddr=0xc8000000\0" \
	"ramdisk_addr_r=0xc9000000\0" \
	"pxefile_addr_r=0xca000000\0" \
	"fdt_addr_r=0xc8800000\0" \
	"fdtfile=" DEVICE_TREE ".dtb\0" \
	"serverip=127.0.0.0\0" \
	BOOTENV

#endif
