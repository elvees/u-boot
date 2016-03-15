/*
 * (C) Copyright 2015
 * ELVEES NeoTek CJSC, <www.elvees-nt.com>
 *
 * Vasiliy Zasukhin <vzasukhin@elvees.com>
 *
 * Configuration settings for the SBC-DBG board
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __SBCDBG_H
#define __SBCDBG_H

#define XTI_FREQ			24000000
#define APLL_VALUE			0x1F
#define CPLL_VALUE			0xF
#define SPLL_VALUE			0x5  /* L1_HCLK = 144 MHz */
#define DIV_SYS0_CTR_VALUE		0
#define DIV_SYS1_CTR_VALUE		0  /* L3_PCLK = L1_HCLK */

/* To enable watchdog define CONFIG_HW_WATCHDOG. Watchdog will be
 * enabled before DDR intialization and will NOT be disabled before
 * booting Linux kernel.
 */

/* #define CONFIG_HW_WATCHDOG */

#include "mcom.h"

#define CONFIG_NR_DRAM_BANKS  		2
#define PHYS_SDRAM_0			CONFIG_SYS_SDRAM_BASE
#define PHYS_SDRAM_0_SIZE		(CONFIG_DDR_SIZE_IN_MB << 20)
#define PHYS_SDRAM_1			0xa0000000
#define PHYS_SDRAM_1_SIZE		(CONFIG_DDR_SIZE_IN_MB << 20)

#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV		0   /* first detected MMC controller */

#define CONFIG_SPL_MMC_SUPPORT
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR	130 /* 65KiB (1KiB reserved + 64KiB max SPL size) */

#if !defined CONFIG_ENV_IS_IN_MMC && \
	!defined CONFIG_ENV_IS_IN_NAND && \
	!defined CONFIG_ENV_IS_IN_FAT && \
	!defined CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_ENV_IS_NOWHERE
#endif

#define CONFIG_SYS_CONSOLE_IS_IN_ENV

#ifndef CONFIG_SPL_BUILD
#include <config_distro_defaults.h>
#define CONFIG_CMD_MEMINFO

#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_BOOTD
#undef CONFIG_CMD_NET
#undef CONFIG_CMD_NFS

/* Enable pre-console buffer to get complete log on the VGA console */
#define CONFIG_PRE_CONSOLE_BUFFER
#define CONFIG_PRE_CON_BUF_SZ		(1024 * 1024)
/* Use the room between the end of bootm_size and the framebuffer */
#define CONFIG_PRE_CON_BUF_ADDR		0x4f000000

/* @fixme We should use bootm instead of bootz. */
/* @fixme Moving fdt is very ugly. There should be way to do it properly. */
#define CONFIG_BOOTCOMMAND \
	"echo \"Loading Linux...\"; " \
	"load mmc 0:1 ${env_addr_r} u-boot.env; " \
	"env import -t ${env_addr_r}; " \
	"load mmc 0:1 ${kernel_addr_r} zImage; " \
	"load mmc 0:1 ${fdt_addr_r} ${fdtfile}; " \
	"fdt addr ${fdt_addr_r}; " \
	"fdt move ${fdt_addr_r} ${newfdt_addr_r} 0x8000; " \
	"fdt addr ${newfdt_addr_r}; " \
	"fdt chosen; " \
	"if test ${ddrctl_cmd} = \"disable\"; then \
		ddrctl ${ddrctl_cmd} ${ddrctl_cid}; fi; " \
	"bootz ${kernel_addr_r} - ${newfdt_addr_r}"

#define MEM_LAYOUT_ENV_SETTINGS \
	"bootm_size=0xf000000\0" \
	"kernel_addr_r=0x40008000\0" \
	"loadaddr=0x40008000\0" \
	"fdtaddr=0x50000000\0" \
	"newfdt_addr_r=0x51000000\0" \
	"fdt_addr_r=0x50000000\0" \
	"mmcdev=0\0" \
	"mmcroot=/dev/mmcblk0p1 rw\0" \
	"mmcrootfstype=ext4 rootwait\0" \
	"root=/dev/mmcblk0p1\0" \
	"scriptaddr=0x43100000\0" \
	"pxefile_addr_r=0x43200000\0" \
	"ramdisk_addr_r=0x43300000\0" \
	"env_addr_r=0x45000000\0" \
	"ddrctl_cid=1\0" \
	"ddrctl_cmd=disable\0"

#define BOOT_TARGET_DEVICES(func) \
    func(MMC, mmc, 0)

#include <config_distro_bootcmd.h>

#ifdef CONFIG_USB_KEYBOARD
#define CONSOLE_STDIN_SETTINGS \
	"preboot=usb start\0" \
	"stdin=serial,usbkbd\0"
#else
#define CONSOLE_STDIN_SETTINGS \
	"stdin=serial\0"
#endif

#ifdef CONFIG_VIDEO
#define CONSOLE_STDOUT_SETTINGS \
	"stdout=serial,vga\0" \
	"stderr=serial,vga\0"
#else
#define CONSOLE_STDOUT_SETTINGS \
	"stdout=serial\0" \
	"stderr=serial\0"
#endif

#define CONSOLE_ENV_SETTINGS \
	CONSOLE_STDIN_SETTINGS \
	CONSOLE_STDOUT_SETTINGS

#define CONFIG_EXTRA_ENV_SETTINGS \
	CONSOLE_ENV_SETTINGS \
	MEM_LAYOUT_ENV_SETTINGS \
	"fdtfile=" CONFIG_FDTFILE "\0" \
	"console=ttyS0,115200\0" \
	BOOTENV

#else /* ifndef CONFIG_SPL_BUILD */
#define CONFIG_EXTRA_ENV_SETTINGS
#endif


#endif /* __SBCDBG_H */
