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

#ifdef CONFIG_TARGET_IPKU
#define APLL_VALUE			0x1F
#define CPLL_VALUE			0x0F
#define SPLL_VALUE			0x05	/* L1_HCLK = 144 MHz */
#define DIV_SYS0_CTR_VALUE		0
#define DIV_SYS1_CTR_VALUE		0	/* L3_PCLK = L1_HCLK */
#else
#define APLL_VALUE			0x1F
#define CPLL_VALUE			0x0F
#define SPLL_VALUE			0x0B	/* L1_HCLK = 288 MHz */
#define DIV_SYS0_CTR_VALUE		0
#define DIV_SYS1_CTR_VALUE		1	/* L3_PCLK = L1_HCLK / 2 */
#endif

#define DIV_DDR0_CTR_VALUE              0
#define DIV_DDR1_CTR_VALUE              0

#define APLL_FREQ			(XTI_FREQ * (APLL_VALUE + 1))
#define CPLL_FREQ			(XTI_FREQ * (CPLL_VALUE + 1))
#define SPLL_FREQ			((XTI_FREQ * (SPLL_VALUE + 1)) >> DIV_SYS0_CTR_VALUE)
#define CONFIG_TIMER_CLK_FREQ		(SPLL_FREQ >> DIV_SYS1_CTR_VALUE)

#include <asm/arch/cpu.h>   /* get chip and board defs */

#define CONFIG_SYS_TEXT_BASE		0x41000000

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

#define CONFIG_NR_DRAM_BANKS		2
#define PHYS_SDRAM_0			CONFIG_SYS_SDRAM_BASE
#define PHYS_SDRAM_0_SIZE		(CONFIG_DDR_SIZE_IN_MB << 20)
#define PHYS_SDRAM_1			0xa0000000
#define PHYS_SDRAM_1_SIZE		(CONFIG_DDR_SIZE_IN_MB << 20)

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

#define CONFIG_BOOTCOMMAND \
	"mmc dev ${mmcdev};" \
	"if mmc rescan; then " \
		"if run loadbootenv; then " \
			"run importbootenv;" \
		"fi;" \
		"if test -n ${bootenvcmd}; then " \
			"run bootenvcmd;" \
		"fi;" \
		"if run mmcload; then " \
			"run mmcboot;" \
		"fi;" \
	"fi;"

#define CONFIG_EXTRA_ENV_SETTINGS \
	"bootm_size=0x10000000\0" \
	"stdin=serial\0" \
	"stdout=serial\0" \
	"stderr=serial\0" \
	"ddrctl_cmd=disable\0" \
	"ddrctl_cid=1\0" \
	"bootenv=u-boot.env\0" \
	"bootenvcmd=\0" \
	"loadbootenv=load mmc ${mmcdev}:${mmcbootpart} ${loadaddr} ${bootenv}\0" \
	"importbootenv=env import -t ${loadaddr} ${filesize}\0" \
	"console=ttyS0,115200\0" \
	"cmdline=\0" \
	"mmcdev=0\0" \
	"mmcbootpart=1\0" \
	"mmcrootpart=2\0" \
	"mmcrootfstype=ext4\0" \
	"mmcargs=setenv bootargs console=${console} " \
		"root=/dev/mmcblk${mmcdev}p${mmcrootpart} " \
		"rootfstype=${mmcrootfstype} rw rootwait ${cmdline}\0" \
	"mmcload=load mmc ${mmcdev}:${mmcbootpart} ${loadaddr} ${bootfile}\0" \
	"mmcboot=run mmcargs; bootz ${loadaddr} - ${fdtcontroladdr}\0"

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
