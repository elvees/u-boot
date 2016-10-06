/*
 * Copyright 2015-2016 ELVEES NeoTek JSC, <www.elvees-nt.com>
 *
 * Vasiliy Zasukhin <vzasukhin@elvees.com>
 * Alexey Kiselev <akiselev@elvees.com>
 *
 * Configuration settings for the MCom platform
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __MCOM_H
#define __MCOM_H

#define XTI_FREQ			24000000

#ifdef CONFIG_TARGET_IPKU
#define APLL_VALUE			0x1F
#define CPLL_VALUE			0x0F
#define SPLL_VALUE			0x05	/* L1_HCLK = 144 MHz */
#define DIV_SYS0_CTR_VALUE		0
#define DIV_SYS1_CTR_VALUE		0	/* L3_PCLK = L1_HCLK */
#else
#define APLL_VALUE			0x1F
#define CPLL_VALUE			0x11
#define SPLL_VALUE			0x0B	/* L1_HCLK = 288 MHz */
#define DIV_SYS0_CTR_VALUE		0
#define DIV_SYS1_CTR_VALUE		1	/* L3_PCLK = L1_HCLK / 2 */
#endif

#define APLL_FREQ			(XTI_FREQ * (APLL_VALUE + 1))
#define CPLL_FREQ			(XTI_FREQ * (CPLL_VALUE + 1))
#define SPLL_FREQ			((XTI_FREQ * (SPLL_VALUE + 1)) >> DIV_SYS0_CTR_VALUE)
#define CONFIG_TIMER_CLK_FREQ		(SPLL_FREQ >> DIV_SYS1_CTR_VALUE)

#define CONFIG_SYS_CACHELINE_SIZE	64

/*
 * High Level Configuration Options
 */
#define CONFIG_MCOM

#include <asm/arch/cpu.h>   /* get chip and board defs */

#define CONFIG_SYS_TEXT_BASE		0x41000000

/*
 * Display CPU information
 */
#define CONFIG_DISPLAY_CPUINFO

/* Serial & console */
#define CONFIG_SYS_NS16550
#define CONFIG_SYS_NS16550_SERIAL
#define CONFIG_SYS_NS16550_CLK		(SPLL_FREQ >> DIV_SYS1_CTR_VALUE)
#define CONFIG_SYS_NS16550_REG_SIZE	-4
#define CONFIG_SYS_NS16550_COM1		0x38028000
#define CONFIG_SYS_NS16550_COM2		0x38029000
#define CONFIG_SYS_NS16550_COM3		0x3802A000
#define CONFIG_SYS_NS16550_COM4		0x3802B000
#ifndef CONFIG_CONS_INDEX
#define CONFIG_CONS_INDEX		1  /* UART0 */
#endif

/* DRAM Base */
#define CONFIG_SYS_SDRAM_BASE		0x40000000
#define CONFIG_SYS_INIT_RAM_ADDR	0x20000000
#define CONFIG_SYS_INIT_RAM_SIZE	0xF000

#define CONFIG_SYS_INIT_SP_OFFSET \
    (CONFIG_SYS_INIT_RAM_SIZE - GENERATED_GBL_DATA_SIZE)
#define CONFIG_SYS_INIT_SP_ADDR \
    (CONFIG_SYS_INIT_RAM_ADDR + CONFIG_SYS_INIT_SP_OFFSET)

#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG

/* Watchdog */
#ifdef CONFIG_HW_WATCHDOG
#define CONFIG_DESIGNWARE_WATCHDOG
#define CONFIG_DW_WDT_BASE		0x38031000
#define CONFIG_DW_WDT_CLOCK_KHZ		((SPLL_FREQ >> DIV_SYS1_CTR_VALUE)/1000)
#define CONFIG_HW_WATCHDOG_TIMEOUT_MS	25000
#endif

/* 4MB of malloc() pool */
#define CONFIG_SYS_MALLOC_LEN		(CONFIG_ENV_SIZE + (4 << 20))

/*
 * Miscellaneous configurable options
 */
#define CONFIG_SYS_CBSIZE		1024    /* Console I/O Buffer Size */
#define CONFIG_SYS_PBSIZE		1024    /* Print Buffer Size */
#define CONFIG_SYS_MAXARGS		16  /* max number of command args */

/* Boot Argument Buffer Size */
#define CONFIG_SYS_BARGSIZE		CONFIG_SYS_CBSIZE

#define CONFIG_SYS_LOAD_ADDR		0x42000000 /* default load address */

/* standalone support */
#define CONFIG_STANDALONE_LOAD_ADDR	0x42000000

/* baudrate */
#define CONFIG_BAUDRATE			115200

/* The stack sizes are set up in start.S using the settings below */
#define CONFIG_STACKSIZE		(256 << 10) /* 256 KiB */

/* FLASH and environment organization */

#define CONFIG_SYS_NO_FLASH

#define CONFIG_SYS_MONITOR_LEN		(512 << 10) /* 512 KiB */

#define CONFIG_ENV_OFFSET		(544 << 10) /* (8 + 24 + 512) KiB */
#define CONFIG_ENV_SIZE			(128 << 10) /* 128 KiB */

#define CONFIG_FAT_WRITE    /* enable write access */

#define CONFIG_NR_DRAM_BANKS		2
#define PHYS_SDRAM_0			CONFIG_SYS_SDRAM_BASE
#define PHYS_SDRAM_0_SIZE		(CONFIG_DDR_SIZE_IN_MB << 20)
#define PHYS_SDRAM_1			0xa0000000
#define PHYS_SDRAM_1_SIZE		(CONFIG_DDR_SIZE_IN_MB << 20)

#define CONFIG_ENV_IS_IN_MMC
#define CONFIG_SYS_MMC_ENV_DEV		0   /* first detected MMC controller */

#define CONFIG_SF_DEFAULT_SPEED		33000000
#define CONFIG_CMD_SF_TEST

#if !defined CONFIG_ENV_IS_IN_MMC && \
	!defined CONFIG_ENV_IS_IN_NAND && \
	!defined CONFIG_ENV_IS_IN_FAT && \
	!defined CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_ENV_IS_NOWHERE
#endif

#define CONFIG_SYS_CONSOLE_IS_IN_ENV

#ifndef CONFIG_SPL_BUILD
#include <config_distro_defaults.h>

/* Enable pre-console buffer to get complete log on the VGA console */
#define CONFIG_PRE_CONSOLE_BUFFER
#define CONFIG_PRE_CON_BUF_SZ		1024
/* Use the room between the end of bootm_size and the framebuffer */
#define CONFIG_PRE_CON_BUF_ADDR		0x4f000000

/* @fixme We should use bootm instead of bootz. */
/* @fixme Moving fdt is very ugly. There should be way to do it properly. */
#define CONFIG_BOOTCOMMAND \
	"echo 'Loading Linux...'; " \
	"load mmc 0:1 ${env_addr_r} u-boot.env; " \
	"size mmc 0:1 u-boot.env; " \
	"env import -t ${env_addr_r} ${filesize}; " \
	"load mmc 0:1 ${kernel_addr_r} zImage; " \
	"fdt addr ${fdtcontroladdr}; " \
	"fdt move ${fdtcontroladdr} ${fdt_addr_r} 0x8000; " \
	"fdt addr ${fdt_addr_r}; " \
	"if test ${ddrctl_cmd} = 'disable'; then " \
		"ddrctl ${ddrctl_cmd} ${ddrctl_cid}; fi; " \
	"bootz ${kernel_addr_r} - ${fdt_addr_r}"

#define MEM_LAYOUT_ENV_SETTINGS \
	"bootm_size=0xf000000\0" \
	"kernel_addr_r=0x40008000\0" \
	"loadaddr=0x40008000\0" \
	"fdtaddr=0x50000000\0" \
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

/* SPL framework */
#define CONFIG_SPL_FRAMEWORK

#define CONFIG_SPL_LIBCOMMON_SUPPORT
#define CONFIG_SPL_LIBDISK_SUPPORT
#define CONFIG_SPL_LIBGENERIC_SUPPORT
#define CONFIG_SPL_GPIO_SUPPORT
#define CONFIG_SPL_MMC_SUPPORT
#define CONFIG_SPL_SERIAL_SUPPORT
#define CONFIG_SPL_SPI_SUPPORT
#define CONFIG_SPL_SPI_FLASH_SUPPORT
#define CONFIG_SPL_SPI_LOAD
#define CONFIG_SPL_WATCHDOG_SUPPORT

#define CONFIG_SPL_LDSCRIPT		"arch/arm/cpu/armv7/u-boot-spl.lds"
#define CONFIG_SPL_MAX_SIZE		0x00010000	/* 64 KB */
#define CONFIG_SPL_TEXT_BASE		0x20000000	/* start of sram */
#define CONFIG_SPL_BSS_START_ADDR	0x4FF80000
#define CONFIG_SPL_BSS_MAX_SIZE		0x00080000	/* 512 KB */
#define CONFIG_SPL_STACK		0x2000F000	/* end of sram */
#define CONFIG_SYS_SPL_MALLOC_START	0x4FF00000
#define CONFIG_SYS_SPL_MALLOC_SIZE	0x00080000	/* 512 KB */

#define CONFIG_SPL_PAD_TO			CONFIG_SPL_MAX_SIZE
#define CONFIG_SYS_SPI_U_BOOT_OFFS		CONFIG_SPL_MAX_SIZE
#define CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR	(2 + CONFIG_SPL_MAX_SIZE / 512)

#endif /* __MCOM_H */
