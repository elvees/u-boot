/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2022 RnD Center "ELVEES", JSC
 *
 * Configuration settings for the MCom-03 architecture
 */

#ifndef __MCOM03_COMMON_H
#define __MCOM03_COMMON_H

#include <linux/sizes.h>

/* Specify available DDR memory.
 * These macros are also used for specifying DDR size available for Linux.
 */
#define CONFIG_SYS_SDRAM_BASE		0x80000000
#define PHYS_SDRAM_0			CONFIG_SYS_SDRAM_BASE
#define PHYS_SDRAM_0_SIZE		SZ_1G
#define CONFIG_SYS_MALLOC_LEN		SZ_8M

#define CONFIG_SYS_INIT_SP_ADDR		0x80400000

#define CONFIG_SYS_LOAD_ADDR		0x82000000 /* default load address */

/* standalone support */
#define CONFIG_STANDALONE_LOAD_ADDR	0x802000000

/* Default environment */
#define CONFIG_LOADADDR			CONFIG_SYS_LOAD_ADDR

/* NAND flash */
#if CONFIG_IS_ENABLED(CMD_NAND)
#define CONFIG_SYS_MAX_NAND_DEVICE      1
#define CONFIG_SYS_NAND_MAX_CHIPS       1
#define CONFIG_SYS_NAND_ONFI_DETECTION
#define CONFIG_SYS_NAND_NO_SUBPAGE_WRITE
#endif

#if CONFIG_IS_ENABLED(CMD_MMC)
#define BOOT_TARGET_DEVICES_MMC(func) \
	func(MMC, mmc, 1) \
	func(MMC, mmc, 0)
#else
#define BOOT_TARGET_DEVICES_MMC(func)
#endif

#if CONFIG_IS_ENABLED(CMD_USB)
#define BOOT_TARGET_DEVICES_USB(func) func(USB, usb, 0)
#else
#define BOOT_TARGET_DEVICES_USB(func)
#endif

#if CONFIG_IS_ENABLED(CMD_PXE)
#define BOOT_TARGET_DEVICES_PXE(func) func(PXE, pxe, na)
#else
#define BOOT_TARGET_DEVICES_PXE(func)
#endif

#define MCOM03_COMMON_ENV_SETTINGS \
	"kernel_addr_r=0x802000000\0" \
	"scriptaddr=0x808000000\0" \
	"ramdisk_addr_r=0x809000000\0" \
	"pxefile_addr_r=0x80a000000\0" \
	"fdt_addr_r=0x808800000\0" \
	"fdtfile=" CONFIG_DEFAULT_DEVICE_TREE ".dtb\0" \
	"serverip=127.0.0.0\0"

#endif
