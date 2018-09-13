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

#if defined(CONFIG_TARGET_IPKU) || defined(CONFIG_TARGET_MCOM02_UKF)
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

#ifdef CONFIG_TARGET_MCOM02_UKF
#define CPLL_VALUE			0x0D
#else
#define CPLL_VALUE			0x0F
#endif  /* CONFIG_TARGET_MCOM02_UKF */

#endif  /* CONFIG_TARGET_SALUTE_PM */

#define DIV_DDR0_CTR_VALUE              0
#define DIV_DDR1_CTR_VALUE              0

#define APLL_FREQ			(XTI_FREQ * (APLL_VALUE + 1))
#define CPLL_FREQ			(XTI_FREQ * (CPLL_VALUE + 1))
#define SPLL_FREQ			((XTI_FREQ * (SPLL_VALUE + 1)) >> DIV_SYS0_CTR_VALUE)
#define CONFIG_TIMER_CLK_FREQ		(SPLL_FREQ >> DIV_SYS1_CTR_VALUE)

#include <asm/arch/cpu.h>   /* get chip and board defs */

#define CONFIG_SYS_SDRAM_BASE		0x40000000

#define CONFIG_NR_DRAM_BANKS		2
#define PHYS_SDRAM_0			CONFIG_SYS_SDRAM_BASE
#define PHYS_SDRAM_0_SIZE		SZ_1G
#define PHYS_SDRAM_1			0xA0000000
#define PHYS_SDRAM_1_SIZE		SZ_1G

/* The first 64 bytes are reserved for the U-Boot image header. */
#define CONFIG_SYS_TEXT_BASE		0x40000040
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
#define CONFIG_DW_WDT_CLOCK_KHZ		((SPLL_FREQ >> DIV_SYS1_CTR_VALUE)/1000)
#define CONFIG_WATCHDOG_TIMEOUT_MSECS	25000
#endif

#ifdef CONFIG_NAND_MCOM02
#define CONFIG_SYS_MAX_NAND_DEVICE	1
#define CONFIG_SYS_NAND_SELF_INIT
#define CONFIG_SYS_NAND_ONFI_DETECTION
#define CONFIG_MTD_DEVICE
#define CONFIG_CMD_MTDPARTS
#define CONFIG_MTD_PARTITIONS
#define CONFIG_RBTREE
#define CONFIG_LZO
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
#define CONFIG_SF_DEFAULT_SPEED		36000000
#define CONFIG_CMD_SF_TEST

/* Environment storage */
#define CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_ENV_OFFSET		SZ_64K
#define CONFIG_ENV_SECT_SIZE		SZ_64K
#define CONFIG_ENV_SIZE			CONFIG_ENV_SECT_SIZE
#define CONFIG_ENV_SPI_MAX_HZ		CONFIG_SF_DEFAULT_SPEED

#ifndef CONFIG_SPL_BUILD
#include <config_distro_defaults.h>

/* Default environment */
#define CONFIG_BOOTFILE			"zImage"
#define CONFIG_LOADADDR			0x40000000

#define CONFIG_PREBOOT \
	"if test ${ddrctl_cmd} = disable; then " \
		"ddrctl ${ddrctl_cmd} ${ddrctl_cid};" \
	"fi;"

#ifdef CONFIG_BOOT_ELF_FROM_SPI
#define CONFIG_BOOTCOMMAND \
	"sf probe ${bootelf_spibus};" \
	"sf read ${bootelf_addr} ${bootelf_spioffset} ${bootelf_elfsize};" \
	"bootelf ${bootelf_addr};"
#else
#define CONFIG_BOOTCOMMAND \
	"if run prep_bootdev; then " \
		"if test -n ${bootenvcmd}; then " \
			"run bootenvcmd;" \
		"fi;" \
		"if run loadbootfile; then " \
			"run mcomboot;" \
		"fi;" \
	"fi;"
#endif

#ifdef CONFIG_TARGET_SALUTE_PM
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
#define VIDEO_MODE "video=HDMI-A-1:1920x1080"
#else
#define VIDEO_MODE
#endif

#define CONFIG_EXTRA_ENV_SETTINGS \
	"bootm_size=0x10000000\0" \
	"stdin=serial\0" \
	"bootelf_spioffset=0x100000\0" \
	"bootelf_spibus=0\0" \
	"bootelf_addr=0x50000000\0" \
	"bootelf_elfsize=0x200000\0" \
	"stdout=serial\0" \
	"stderr=serial\0" \
	DDRCTL_CMD \
	"ddrctl_cid=1\0" \
	"bootenvcmd=\0" \
	"console=ttyS0,115200\0" \
	"cmdline=" BLACKLIST VIDEO_MODE "\0" \
	"bootsource=mmc\0" \
	"mmcdev=0\0" \
	"bootpartnum=1\0" \
	"rootpartnum=2\0" \
	"rootfstype=ext4\0" \
	"rootfsdev=\0" \
	"loadcmd=load\0" \
	"loaddev=\0" \
	"loadpart=\0" \
	"prep_bootdev=" \
		"if test ${bootsource} = usb; then " \
			"rootfsdev=/dev/sda${rootpartnum};" \
			"loaddev=${bootsource};" \
			"loadpart=0:${bootpartnum};" \
			"usb start;" \
		"elif test ${bootsource} = mmc; then " \
			"rootfsdev=/dev/mmcblk${mmcdev}p${rootpartnum};" \
			"loaddev=${bootsource};" \
			"loadpart=${mmcdev}:${bootpartnum};" \
			"mmc dev ${mmcdev};"\
			"mmc rescan;" \
		"elif test ${bootsource} = nand; then " \
			"rootfsdev=ubi0:root;" \
			"setenv loadcmd ubifsload;" \
			"setenv rootfstype ubifs;" \
			"setenv cmdline ${cmdline} ubi.mtd=arasan_nfc;" \
			"mtdparts default;" \
			"ubi part allnand;" \
			"ubifsmount ubi:boot;" \
		"fi;\0" \
	"loadbootfile=${loadcmd} ${loaddev} ${loadpart} " \
		"${loadaddr} ${bootfile}\0" \
	"set_bootargs=setenv bootargs console=${console} " \
		"root=${rootfsdev} rootfstype=${rootfstype} rw rootwait " \
		"${cmdline}\0" \
	"mcomboot=run set_bootargs;bootz ${loadaddr} - ${fdtcontroladdr}\0"
#endif

#define CONFIG_USB_DWC2
#ifdef CONFIG_CMD_USB_MASS_STORAGE
#define CONFIG_USB_FUNCTION_MASS_STORAGE
#endif

/* SPL framework */
#define CONFIG_SPL_FRAMEWORK
#define CONFIG_SPL_SPI_LOAD

#define CONFIG_SPL_MAX_SIZE		0x0000E000	/* 56 KB */
#define CONFIG_SPL_TEXT_BASE		0x20000000
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
