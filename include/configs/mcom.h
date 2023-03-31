/*
 * Copyright 2015-2016 ELVEES NeoTek JSC, <www.elvees-nt.com>
 * Copyright 2017 RnD Center "ELVEES", JSC
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

#include <linux/sizes.h>

#define XTI_FREQ			24000000

#if defined(CONFIG_TARGET_IPKU) || defined(CONFIG_TARGET_IPKU2)
#define APLL_VALUE			0x1F
#define SPLL_VALUE			0x05	/* L1_HCLK = 144 MHz */
#define DIV_SYS0_CTR_VALUE		0
#define DIV_SYS1_CTR_VALUE		0	/* L3_PCLK = L1_HCLK */
#else
#define APLL_VALUE			0x1F
#define SPLL_VALUE			0x0B	/* L1_HCLK = 288 MHz */
#define DIV_SYS0_CTR_VALUE		0
#define DIV_SYS1_CTR_VALUE		1	/* L3_PCLK = L1_HCLK / 2 */
#endif

#if defined(CONFIG_TARGET_SALUTE_PM) || defined(CONFIG_TARGET_SALUTE_D2)
#define CPLL_VALUE			0x14
#else

#if defined(CONFIG_TARGET_MCOM02_UKF) || defined(CONFIG_TARGET_IPKU) || \
	defined(CONFIG_TARGET_IPKU2)
#define CPLL_VALUE			0x0D
#else

#if defined(CONFIG_TARGET_ECAM02DM) || defined(CONFIG_TARGET_ECAM02DM3)
#define CPLL_VALUE			0x10	/* 408 MHz */
#else
#define CPLL_VALUE			0x0F
#endif  /* CONFIG_TARGET_ECAM02DM */

#endif  /* CONFIG_TARGET_MCOM02_UKF||CONFIG_TARGET_IPKU||CONFIG_TARGET_IPKU2 */

#endif  /* CONFIG_TARGET_SALUTE_PM */

#define DIV_DDR0_CTR_VALUE              0
#define DIV_DDR1_CTR_VALUE              0

#define APLL_FREQ	(XTI_FREQ * (APLL_VALUE + 1))
#define CPLL_FREQ	(XTI_FREQ * (CPLL_VALUE + 1))
#define SPLL_FREQ	((XTI_FREQ * (SPLL_VALUE + 1)) >> DIV_SYS0_CTR_VALUE)
#define TIMER_FREQ	(SPLL_FREQ >> DIV_SYS1_CTR_VALUE)

#include <asm/arch/cpu.h>   /* get chip and board defs */

#define CONFIG_SYS_SDRAM_BASE		0x40000000

#define PHYS_SDRAM_0			CONFIG_SYS_SDRAM_BASE
#if defined(CONFIG_TARGET_ECAM02DM)
#define PHYS_SDRAM_0_SIZE		SZ_256M
#elif defined(CONFIG_TARGET_ECAM02DM3)
#define PHYS_SDRAM_0_SIZE		SZ_128M
#else
#define PHYS_SDRAM_0_SIZE		SZ_1G
#endif
#define PHYS_SDRAM_1			0xA0000000
#if defined(CONFIG_TARGET_ECAM02DM)
#define PHYS_SDRAM_1_SIZE		SZ_256M
#elif defined(CONFIG_TARGET_ECAM02DM3)
/* For now the second LPDDR2 is not installed on the board, but we
 * determine the size of the second LPDDR2 in case it is installed.
 */
#define PHYS_SDRAM_1_SIZE		SZ_128M
#else
#define PHYS_SDRAM_1_SIZE		SZ_1G
#endif

/* The first 64 bytes are reserved for the U-Boot image header. */
#define CONFIG_SYS_INIT_SP_ADDR		0x40400000

#define CONFIG_SYS_MALLOC_LEN		SZ_8M

/* I2C support */
#ifdef CONFIG_TARGET_SALUTE_PM
#define CONFIG_SYS_I2C
#define CONFIG_SYS_I2C_BASE		0x3802C000
#define CONFIG_SYS_I2C_SPEED		100000
#define IC_CLK				(SPLL_FREQ / 1000000)

/* Power support */
#define CONFIG_POWER
#define CONFIG_POWER_I2C
#define CONFIG_POWER_PFUZE100
#define CONFIG_POWER_PFUZE100_I2C_ADDR	0x08
#endif

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

#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG

/* Watchdog */
#ifdef CONFIG_HW_WATCHDOG
#define CONFIG_DESIGNWARE_WATCHDOG
#define CONFIG_DW_WDT_BASE		0x38031000
#define CONFIG_DW_WDT_CLOCK_KHZ	((SPLL_FREQ >> DIV_SYS1_CTR_VALUE) / 1000)
#define CONFIG_WATCHDOG_TIMEOUT_MSECS	25000
#endif

#ifdef CONFIG_NAND_MCOM02
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_ONFI_DETECTION
#define CONFIG_MTD_DEVICE
#define CONFIG_CMD_MTDPARTS
#define MTDIDS_DEFAULT		"nand0=mcom02-nand"
#define MTDPARTS_DEFAULT	"mtdparts=mcom02-nand:-(allnand)"
#endif

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

/* Serial Flash support */
#define CONFIG_CMD_SF_TEST

/* Environment storage */
#define CONFIG_ENV_OFFSET		SZ_64K
#define CONFIG_ENV_SECT_SIZE		SZ_64K
#define CONFIG_ENV_SIZE			CONFIG_ENV_SECT_SIZE
#define CONFIG_ENV_SPI_MAX_HZ		CONFIG_SF_DEFAULT_SPEED

#ifndef CONFIG_SPL_BUILD

/* Default environment */
#if defined(CONFIG_TARGET_ECAM02DM) || defined(CONFIG_TARGET_ECAM02DM3)
#define CONFIG_BOOTFILE			"/boot/zImage"
#else
#define CONFIG_BOOTFILE			"zImage"
#endif  /* CONFIG_TARGET_ECAM02DM || CONFIG_TARGET_ECAM02DM3 */

#define CONFIG_LOADADDR			0x40000000

#define CONFIG_PREBOOT \
	"if test -n \"${serial#}\"; then " \
		"echo \"Board serial: ${serial#}\";" \
	"fi;"

#undef CONFIG_BOOTCOMMAND
#ifdef CONFIG_BOOT_ELF_FROM_SPI
#define CONFIG_BOOTCOMMAND \
	"sf probe ${bootelf_spibus};" \
	"sf read ${bootelf_addr} ${bootelf_spioffset} ${bootelf_elfsize};" \
	"bootelf ${bootelf_addr};"
#else
#define CONFIG_BOOTCOMMAND \
	"run findfdt; run distro_bootcmd;"
#endif

#define BOOTENV_DEV_LEGACY_MMC		BOOTENV_DEV_BLKDEV
#define BOOTENV_DEV_LEGACY_USB		BOOTENV_DEV_BLKDEV
#define BOOTENV_DEV_LEGACY_UBIFS	BOOTENV_DEV_BLKDEV

#define BOOTENV_DEV_NAME_LEGACY_MMC	BOOTENV_DEV_NAME_BLKDEV
#define BOOTENV_DEV_NAME_LEGACY_USB	BOOTENV_DEV_NAME_BLKDEV
#define BOOTENV_DEV_NAME_LEGACY_UBIFS	BOOTENV_DEV_NAME_BLKDEV

#if defined(CONFIG_TARGET_ECAM02DM) || defined(CONFIG_TARGET_ECAM02DM3)
#define BOOTENV_DEV_ECAM02DM		BOOTENV_DEV_BLKDEV
#define BOOTENV_DEV_NAME_ECAM02DM	BOOTENV_DEV_NAME_BLKDEV
#define BOOT_TARGET_DEVICES(func)	func(ECAM02DM, ecam02dm, 0)
#else
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 0) \
	func(LEGACY_MMC, legacy_mmc, 0) \
	func(MMC, mmc, 1) \
	func(LEGACY_MMC, legacy_mmc, 1) \
	func(USB, usb, 0) \
	func(LEGACY_USB, legacy_usb, 0) \
	func(UBIFS, ubifs, 0) \
	func(LEGACY_UBIFS, legacy_ubifs, 0)
#endif  /* CONFIG_TARGET_ECAM02DM || CONFIG_TARGET_ECAM02DM3 */

#include <config_distro_bootcmd.h>

#if defined(CONFIG_TARGET_SALUTE_PM) || defined(CONFIG_TARGET_ECAM02DM) || \
defined(CONFIG_TARGET_ECAM02DM3)
#define DDRCTL_CMD "ddrctl_cmd=enable\0"
#else
#define DDRCTL_CMD "ddrctl_cmd=disable\0"
#endif

#if defined(CONFIG_TARGET_SALUTE_D1) || defined(CONFIG_TARGET_SALUTE_D2)
#define BLACKLIST "modprobe.blacklist=vpoutfb"
#else
#define BLACKLIST
#endif

#ifdef CONFIG_TARGET_SALUTE_PM
#define VIDEO_MODE "video=HDMI:1920x1080"
#else
#define VIDEO_MODE
#endif

#ifdef CONFIG_FIT
#define KERNEL_ADDR_R "kernel_addr_r=0xC0000000\0"
#else
#define KERNEL_ADDR_R "kernel_addr_r=0x40000000\0"
#endif

#if defined(CONFIG_TARGET_ECAM02DM) || defined(CONFIG_TARGET_ECAM02DM3)
#define ROOTFS_OPTIONS "ro UPPER_DEV=/dev/ubi0_2 init=/sbin/init-overlay-rootfs.sh"

#define BOOTUBIVOL "bootubivol=system_a\0" \
	"safe_bootubivol=system_a\0"

#define EXTRA_BOOTENV "ecam02dm_boot=" \
	"if test \"${bootubivol}\" != \"${safe_bootubivol}\"; then " \
		"if env exists tried_to_boot; then " \
			"setenv bootubivol ${safe_bootubivol};" \
			"setenv tried_to_boot;" \
			"saveenv;" \
		"else " \
			"setenv tried_to_boot true;" \
			"saveenv;" \
		"fi;" \
	"fi;" \
	"setenv rootfsdev ubi0:${bootubivol};" \
	"setenv loaddev ubi;" \
	"setenv loadpart ubi:${bootubivol};"\
	"setenv rootfstype ubifs;" \
	"setenv cmdline ${cmdline} ubi.mtd=arasan_nfc;" \
	"mtdparts default;" \
	"ubi part ${bootubipart};" \
	"ubifsmount ubi:${bootubivol};" \
	"run legacy_bootcmd\0"

#define EXTRA_CMDLINE " ubi.fm_autoconvert=1 vinc.cacheable=1 panic=1"
#else
#define ROOTFS_OPTIONS "rw"

#define BOOTUBIVOL

#define EXTRA_BOOTENV

#define EXTRA_CMDLINE
#endif  /* CONFIG_TARGET_ECAM02DM || CONFIG_TARGET_ECAM02DM3 */

#ifndef FDTFILE
#define FDTFILE CONFIG_DEFAULT_DEVICE_TREE
#endif

#define FDTFILE_ENV "fdtfile=" __stringify(FDTFILE) ".dtb\0"

#define CONFIG_EXTRA_ENV_SETTINGS \
	"bootm_size=0x8000000\0" \
	"stdin=serial\0" \
	"bootelf_spioffset=0x100000\0" \
	"bootelf_spibus=0\0" \
	"bootelf_addr=0x50000000\0" \
	"bootelf_elfsize=0x200000\0" \
	"bootfile=" CONFIG_BOOTFILE "\0" \
	"stdout=serial\0" \
	"stderr=serial\0" \
	DDRCTL_CMD \
	"ddrctl_cid=1\0" \
	"bootenvcmd=\0" \
	"console=ttyS0,115200\0" \
	"rootfs_options=" ROOTFS_OPTIONS "\0" \
	"cmdline=" BLACKLIST VIDEO_MODE EXTRA_CMDLINE "\0" \
	"bootpartnum=1\0" \
	"rootpartnum=2\0" \
	"usb_pgood_delay=5000\0" \
	"bootubipart=allnand\0" \
	BOOTUBIVOL \
	"loadbootfile=load ${loaddev} ${loadpart} ${loadaddr} ${bootfile}\0" \
	"set_bootargs=setenv bootargs console=${console} " \
		"root=${rootfsdev} rootfstype=${rootfstype} rootwait " \
		"${rootfs_options} ${cmdline}\0" \
	"mcomboot=run set_bootargs;bootz ${loadaddr} - ${fdtcontroladdr}\0" \
	"legacy_bootcmd=" \
		"if test -n ${bootenvcmd}; then " \
			"run bootenvcmd;" \
		"fi;" \
		"if run loadbootfile; then " \
			"run mcomboot;" \
		"fi;\0" \
	"legacy_mmc_boot=" \
		"setenv rootfsdev /dev/mmcblk${devnum}p${rootpartnum};" \
		"setenv loaddev mmc;" \
		"setenv loadpart ${devnum}:${bootpartnum};" \
		"setenv rootfstype ext4;" \
		"mmc dev ${devnum};" \
		"mmc rescan;" \
		"run legacy_bootcmd\0" \
	"legacy_usb_boot=" \
		"setenv rootfsdev /dev/sda${rootpartnum};" \
		"setenv loaddev usb;" \
		"setenv loadpart 0:${bootpartnum};" \
		"setenv rootfstype ext4;" \
		"usb start;" \
		"run legacy_bootcmd\0" \
	"legacy_ubifs_boot=" \
		"setenv rootfsdev ubi0:root;" \
		"setenv loaddev ubi;" \
		"setenv loadpart ubi:boot;" \
		"setenv rootfstype ubifs;" \
		"setenv cmdline ${cmdline} ubi.mtd=arasan_nfc;" \
		"mtdparts default;" \
		"ubi part ${bootubipart};" \
		"ubifsmount ubi:boot;" \
		"run legacy_bootcmd\0" \
	"fdt_addr_r=0x46000000\0" \
	"pxefile_addr_r=0x47000000\0" \
	"scriptaddr=0x47000000\0" \
	KERNEL_ADDR_R \
	"ramdisk_addr_r=0x50000000\0" \
	"findfdt=" \
		"setenv fdt_addr ${fdtcontroladdr};\0" \
	FDTFILE_ENV \
	BOOTENV \
	EXTRA_BOOTENV
#endif

/* SPL framework */
#define CONFIG_SPL_SPI_LOAD

#define CONFIG_SPL_MAX_SIZE		0x0000E000	/* 56 KB */
#define CONFIG_SPL_STACK		0x2000F400
#define CONFIG_SYS_SPL_MALLOC_START	0x40380000
#define CONFIG_SYS_SPL_MALLOC_SIZE	0x00080000	/* 512 KB */

/* For writing into SPI flash the U-Boot image is appended to the SPL image.
 * The SPL image is located in the first erase sector. The second sector is
 * reserved for the environment storage. The padding is used to align the
 * U-Boot image to the third sector boundary. */
#define CONFIG_SPL_PAD_TO			0x00020000
#define CONFIG_SYS_SPI_U_BOOT_OFFS		CONFIG_SPL_PAD_TO

#define CONFIG_BUILD_TARGET		"u-boot.mcom"

#endif /* __MCOM_H */
