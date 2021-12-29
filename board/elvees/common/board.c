/*
 * Copyright 2012-2013 Henrik Nordstrom <henrik@henriknordstrom.net>
 * Copyright 2013 Luke Kenneth Casson Leighton <lkcl@lkcl.net>
 * Copyright 2007-2011 Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * Copyright 2015-2016 ELVEES NeoTek JSC
 * Copyright 2017-2018 RnD Center "ELVEES", JSC
 *
 * Common board initialization code for ELVEES MCom-compatible boards.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#ifdef CONFIG_SPL_BUILD
#include <spl.h>
#include <asm/arch/bootrom.h>
#include <asm/arch/ddr.h>
#include <asm/arch/ddr-calibration.h>
#endif
#include <asm/arch/clock.h>
#include <asm/arch/regs.h>
#include <asm/io.h>
#include <environment.h>
#include <linux/kernel.h>
#include <spi.h>
#include <spi_flash.h>
#include <usb.h>
#include <usb/dwc2_udc.h>

DECLARE_GLOBAL_DATA_PTR;

/* Struct is similar to env_t but env_t has additional field if
 * CONFIG_SYS_REDUNDAND_ENVIRONMENT is defined */
struct factory_env {
	u32 crc;
	unsigned char data[];
};

int board_init(void)
{
	return 0;
}

static bool is_ddrmc_active(int ddrmc_id)
{
	cmctr_t *CMCTR = (cmctr_t *)CMCTR_BASE;
	ddrmc_t *DDRMC;
	bool clk_en;

	u32 reg = CMCTR->GATE_CORE_CTR;

	switch (ddrmc_id) {
	case 0:
		DDRMC = (ddrmc_t *)DDRMC0_BASE;
		clk_en = (reg >> 1) & 1;
		break;
	case 1:
		DDRMC = (ddrmc_t *)DDRMC1_BASE;
		clk_en = (reg >> 2) & 1;
		break;
	default:
		puts("is_ddrmc_active: Invalid argument\n");
		return 0;
	}

	/* If DDRMC clock disabled then DDRMC is not active.
	 * If clock enabled we should additionally check
	 * STAT register.
	 */

	if (clk_en && (DDRMC->STAT == 0x1))
		return 1;

	return 0;
}

int dram_init_banksize(void)
{
	/* DDR controller #0 is active since TPL uses it */
	gd->bd->bi_dram[0].start = PHYS_SDRAM_0;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_0_SIZE;

	/* If DDR controller #1 is active increase
	 * available for Linux memory size
	 */
	if (is_ddrmc_active(1)) {
		gd->bd->bi_dram[1].start = PHYS_SDRAM_1;
		gd->bd->bi_dram[1].size = PHYS_SDRAM_1_SIZE;
	} else {
		gd->bd->bi_dram[1].start = 0;
		gd->bd->bi_dram[1].size = 0;
	}

	return 0;
}

__weak int ddr_poweron(void)
{
	return 0;
}

#ifdef CONFIG_SPL_BUILD
#ifdef CONFIG_PRINT_DDR_PARAMS
static void print_ddr_params(struct ddr_cfg *cfgs)
{
	int i;

	for (i = 0; i < 2; i++) {
		printf("DDR #%d parameters: ", i);
		printf("ods_mc=%d, ods_dram=%d, odt_mc=%d, odt_dram=%d\n",
		       cfgs[i].impedance.ods_mc, cfgs[i].impedance.ods_dram,
		       cfgs[i].impedance.odt_mc, cfgs[i].impedance.odt_dram);
	}
}
#endif
#endif

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_0_SIZE;
#ifdef CONFIG_SPL_BUILD
	struct ddr_cfg cfg[2];
	struct ddr_freq freq;
	int i, ret;

	ret = ddr_poweron();
	if (ret) {
		printf("Failed to set DDR power: %d\n", ret);
		hang();
	}

	freq.xti_freq = XTI_FREQ;
	freq.cpll_mult = CPLL_VALUE;
	freq.ddr0_div = DIV_DDR0_CTR_VALUE;
	freq.ddr1_div = DIV_DDR1_CTR_VALUE;

	for (i = 0; i < 2; i++) {
		ret = set_sdram_cfg(&cfg[i],
				    ddr_get_clock_period(i, &freq));
		if (ret)
			return ret;
		cfg[i].ctl_id = i;
	}

#ifdef CONFIG_DDR_CALIBRATION
	set_calib_cfg(cfg);
#endif
	timer_init();

	puts("DDR controllers init started\n");

	u32 status = mcom_ddr_init(&cfg[0], &cfg[1], &freq);

	for (i = 0; i < 2; i++) {
		const char *result_str[2] = {"done", "failed"};

		printf("DDR controller #%d init %s\n",
		       i, result_str[!!ddr_getrc(status, i)]);
	}
	printf("DDR frequency: %d MHz\n", CPLL_FREQ / 1000000);
#ifdef CONFIG_PRINT_DDR_PARAMS
	print_ddr_params(cfg);
#endif

	return status;
#else
	return 0;
#endif
}

#ifdef CONFIG_SPL_BUILD
u32 spl_boot_device(void)
{
	const smctr_t *SMCTR = (smctr_t *)SMCTR_BASE;
	const u32 boot = SMCTR->BOOT;
	int sdnum;

	switch (boot) {
	case SMCTR_BOOT_SPI0:
		return BOOT_DEVICE_SPI;
	case SMCTR_BOOT_SDMMC0:
		sdnum = bootrom_sd_get_nom();
		switch (sdnum) {
		case 0:
			return BOOT_DEVICE_MMC1;
		case 1:
			return BOOT_DEVICE_MMC2;
		default:
			printf("Unexpected SDMMC controller number: %d\n",
			       sdnum);
		}
		break;
	default:
		printf("Unexpected boot device in SMCTR: %u\n", boot);
	}

	return BOOT_DEVICE_NONE;
}

u32 spl_boot_mode(const u32 boot_device)
{
	switch (boot_device) {
	case BOOT_DEVICE_MMC1:
	case BOOT_DEVICE_MMC2:
		return MMCSD_MODE_RAW;
	}

	return MMCSD_MODE_UNDEFINED;
}
#endif

static struct dwc2_plat_otg_data mcom02_otg_data;

int board_usb_init(int index, enum usb_init_type init)
{
	int node;
	unsigned long int addr;
	const void *blob = gd->fdt_blob;

	if (init == USB_INIT_DEVICE) {
		node = fdt_node_offset_by_compatible(blob, -1, "snps,dwc2");

		if (node <= 0) {
			printf("No USB Device Controller found\n");
			return -ENODEV;
		}

		addr = fdtdec_get_addr(blob, node, "reg");
		if (addr == FDT_ADDR_T_NONE) {
			printf("UDC has no 'reg' property\n");
			return -EINVAL;
		}

		mcom02_otg_data.regs_otg = addr;

		return dwc2_udc_probe(&mcom02_otg_data);
	}
	return 0;
}

#if defined(CONFIG_MISC_INIT_R) && defined(CONFIG_SPL_SPI_FLASH_SUPPORT)
static struct factory_env *read_factory_settings(u32 *data_size)
{
	struct spi_flash *flash;
	struct factory_env *ptr = NULL;
	u32 factorysize;
	u32 factoryoffset;

	flash = spi_flash_probe(CONFIG_SF_DEFAULT_BUS, CONFIG_SF_DEFAULT_CS,
				CONFIG_SF_DEFAULT_SPEED,
				CONFIG_SF_DEFAULT_MODE);
	if (!flash) {
		printf("Unable to probe SPI flash\n");
		return NULL;
	}
	/* Factory settings are contained in the last eraseblock */
	factorysize = flash->erase_size;
	factoryoffset = flash->size - flash->erase_size;

	/* factorysize and factoryoffset variables are needed for change
	 * factory settings from U-Boot command line */
	env_set_hex("factorysize", factorysize);
	env_set_hex("factoryoffset", factoryoffset);
	ptr = (struct factory_env *)malloc(factorysize);
	if (!ptr) {
		printf("Not enough memory\n");
		goto flash_free;
	}
	if (spi_flash_read(flash, factoryoffset, factorysize, ptr)) {
		printf("Unable to read factory settings from SPI flash\n");
		free(ptr);
		ptr = NULL;
		goto flash_free;
	}
	*data_size = factorysize;

flash_free:
	spi_flash_free(flash);
	return ptr;
}

int load_factory_settings(void)
{
	struct factory_env *factory_env;
	u32 data_size;
	u32 real_crc;

	printf("Loading factory settings from SPI Flash... ");
	factory_env = read_factory_settings(&data_size);
	if (!factory_env)
		return -ENODATA;

	data_size -= sizeof(real_crc);
	real_crc = crc32(0, factory_env->data, data_size);
	if (real_crc != factory_env->crc) {
		/* If SPI flash block (include CRC) is 0xff it means that
		 * block is empty. If data (include CRC) is not 0xff it means
		 * that data is damaged. */
		if (real_crc != (u32)-1)
			printf("CRC error in factory settings\n");

		/* Factory settings not found or corrupted. Nothing to import */
		free(factory_env);
		return -ENODATA;
	}

	if (!himport_r(&env_htab, (char *)factory_env->data, data_size, '\0',
		       H_NOCLEAR, 0, 0, NULL)) {
		printf("Unable to import factory settings\n");
		free(factory_env);
		return -ENODATA;
	}

	/* If ethaddr variable does not exists then create and set it using
	 * address from factory_eth_mac */
	if (!env_get("ethaddr")) {
		char *factory_eth_mac = env_get("factory_eth_mac");

		if (factory_eth_mac)
			env_set("ethaddr", factory_eth_mac);
	}

	return 0;
}

int misc_init_r(void)
{
	/* Check if environment been saved */
	if (!env_get("factory_serial")) {
		if (load_factory_settings())
			env_set("factory_serial", "0");

		printf("*** First boot, saving environment...\n");
		env_save();
	}

	return 0;
}
#endif
