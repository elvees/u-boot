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
#define CFG_SYS_SDRAM_BASE		0x890400000
#define PHYS_SDRAM_0			CFG_SYS_SDRAM_BASE
#define PHYS_SDRAM_0_SIZE		SZ_256M

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

#if CONFIG_IS_ENABLED(CMD_DHCP)
#define BOOT_TARGET_DEVICES_DHCP(func) func(DHCP, dhcp, na)
#else
#define BOOT_TARGET_DEVICES_DHCP(func)
#endif

/*
 * Define `BOOTENV_EFI_SET_FDTFILE_FALLBACK` will be used in
 * include/config_distro_bootcmd.h file.
 */
#define BOOTENV_EFI_SET_FDTFILE_FALLBACK                                  \
	"if test -z \"${fdtfile}\" -a -n \"${soc}\"; then "               \
	  "setenv efi_fdtfile ${soc}-${board}${boardver}.dtb; "           \
	"fi; "

#define MCOM03_COMMON_ENV_SETTINGS \
	"kernel_addr_r=0x892400000\0" \
	"scriptaddr=0x898400000\0" \
	"ramdisk_addr_r=0x899400000\0" \
	"pxefile_addr_r=0x89a400000\0" \
	"fdt_addr_r=0x898c00000\0" \
	"serverip=127.0.0.0\0"

#endif
