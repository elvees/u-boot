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
#if IS_ENABLED(CONFIG_TARGET_HAPS)
#define CFG_SYS_SDRAM_BASE		0xC0000000
#define PHYS_SDRAM_0			CFG_SYS_SDRAM_BASE
#define PHYS_SDRAM_0_SIZE		SZ_1G
#define PHYS_SDRAM_1			0x80000000
#define PHYS_SDRAM_1_SIZE		SZ_1G
#else
#define CFG_SYS_SDRAM_BASE		0x890400000
#define PHYS_SDRAM_0			CFG_SYS_SDRAM_BASE
#define PHYS_SDRAM_0_SIZE		SZ_1G
/* Only used for HAPS */
#define PHYS_SDRAM_1			0x0
#define PHYS_SDRAM_1_SIZE		0
#endif

#if IS_ENABLED(CONFIG_CMD_MMC)
#define BOOT_TARGET_DEVICES_MMC(func) \
	func(MMC, mmc, 1) \
	func(MMC, mmc, 0)
#define MCOM03_BOOT_FIT_MMC_CMD \
	"bootcmd_mmc1=" \
		"rootfsdev=/dev/mmcblk1p1; " \
		"bootdev=mmc; " \
		"bootpart=1:1; " \
		"mmc dev 1; " \
		"mmc rescan; " \
		"run bootscript\0" \
	"bootcmd_mmc0=" \
		"rootfsdev=/dev/mmcblk0p1; " \
		"bootdev=mmc; " \
		"bootpart=0:1;" \
		"mmc dev 0; " \
		"mmc rescan; " \
		"run bootscript\0"
#define MCOM03_BOOT_FIT_MMC_TARGETS "mmc1 mmc0"
#else
#define BOOT_TARGET_DEVICES_MMC(func)
#define MCOM03_BOOT_FIT_MMC_CMD
#define MCOM03_BOOT_FIT_MMC_TARGETS
#endif

#if IS_ENABLED(CONFIG_CMD_USB)
#define BOOT_TARGET_DEVICES_USB(func) func(USB, usb, 0)
#define MCOM03_BOOT_FIT_USB_CMD \
	"bootcmd_usb0=" \
		"rootfsdev=/dev/sda1; " \
		"bootdev=usb; " \
		"bootpart=0:1; " \
		"usb start; " \
		"run bootscript\0"
#define MCOM03_BOOT_FIT_USB_TARGETS "usb0"
#else
#define BOOT_TARGET_DEVICES_USB(func)
#define MCOM03_BOOT_FIT_USB_CMD
#define MCOM03_BOOT_FIT_USB_TARGETS
#endif

#if IS_ENABLED(CONFIG_CMD_PXE)
#define BOOT_TARGET_DEVICES_PXE(func) func(PXE, pxe, na)
#else
#define BOOT_TARGET_DEVICES_PXE(func)
#endif

#if IS_ENABLED(CONFIG_CMD_DHCP)
#define BOOT_TARGET_DEVICES_DHCP(func) func(DHCP, dhcp, na)
#else
#define BOOT_TARGET_DEVICES_DHCP(func)
#endif

#if IS_ENABLED(CONFIG_PANIC_HANG)
#define MCOM03_BOOT_FIT_PANIC_CMD "bootcmd_panic=panic\0"
#define MCOM03_BOOT_FIT_PANIC_TARGETS "panic"
#else
#define MCOM03_BOOT_FIT_PANIC_CMD
#define MCOM03_BOOT_FIT_PANIC_TARGETS
#endif

/* Some Ethernet PHY (ex. DP83867) needs more than 20 seconds timeout for
 * autonegotiation if link downshift is used.
 */
#define PHY_ANEG_TIMEOUT		30000

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
	"fdt_addr_r=0x898c00000\0" \
	"fdtoverlay_addr_r=0x899300000\0" \
	"pxefile_addr_r=0x899400000\0" \
	"fit_addr_r=0x899500000\0" \
	"ramdisk_addr_r=0x8a1500000\0" \
	"serverip=127.0.0.0\0"

#if IS_ENABLED(CONFIG_FIT)
#define BOOTENV \
	"bootfile=/boot/Image.fit\0" \
	"boot_targets=" MCOM03_BOOT_FIT_MMC_TARGETS " " \
			MCOM03_BOOT_FIT_USB_TARGETS " " \
			MCOM03_BOOT_FIT_PANIC_TARGETS "\0" \
	"mcom03_bootcmd=" \
		"for target in ${boot_targets}; do " \
			"run bootcmd_${target}; " \
		"done\0" \
	"bootcmd=run mcom03_bootcmd\0" \
	"bootscript=" \
		"echo \"Booting ${bootfile} from ${bootdev} ${bootpart} ...\"; " \
		"if load ${bootdev} ${bootpart} ${fit_addr_r} ${bootfile}; then " \
			"setenv bootargs console=ttyS0,115200n8 earlycon " \
				"root=${rootfsdev} rootfstype=ext4 rw rootwait; " \
			"bootm ${fit_addr_r}; " \
		"fi; " \
		"echo FAILED to boot ${bootfile}: continuing...\0" \
	MCOM03_BOOT_FIT_MMC_CMD \
	MCOM03_BOOT_FIT_USB_CMD \
	MCOM03_BOOT_FIT_PANIC_CMD
#endif

#endif
