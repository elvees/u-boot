/*
 * Copyright 2012-2013 Henrik Nordstrom <henrik@henriknordstrom.net>
 * Copyright 2013 Luke Kenneth Casson Leighton <lkcl@lkcl.net>
 * Copyright 2007-2011 Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * Copyright 2015-2016 ELVEES NeoTek JSC, <www.elvees-nt.com>
 *
 * Some board init for the Elvees MCom-compatible board.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <mmc.h>
#ifdef CONFIG_SPL_BUILD
#include <spl.h>
#include <asm/arch/bootrom.h>
#endif
#include <asm/arch/clock.h>
#include <asm/arch/ddr_cfg.h>
#include <asm/arch/regs.h>
#include <asm/arch/sdmmc.h>
#include <asm/io.h>
#include <linux/kernel.h>

DECLARE_GLOBAL_DATA_PTR;

#define tCK DIV_ROUND_UP(1000000, ((CPLL_VALUE + 1) * (XTI_FREQ / 1000000)))
#define bl 8 /* burst length */

/* The following parameters are for Nanya NT5CB256M8FN-DI DRAM. */

#define cl 7 /* read latency. Units: clocks cycles */
#define cwl 6 /* write latency. Units: clock cycles */
#define tWR 15000 /* Units: ps */
#define tFAW 30000 /* Units: ps */
#define tRAS_min 35000 /* Units: ps */
#define tRAS_max (9 * 7800000) /* Units: ps */
#define tRC 48750 /* Units: ps */
#define tXPDLL max(10 * tCK, 24000) /* Units: ps */
#define tAL 0
#define tRTP max(4 * tCK, 7500) /* Units: ps */
#define tWTR max(4 * tCK, 7500) /* Units: ps */
#define tRCD 13750 /* Units: ps */
#define tRRD max(4 * tCK, 6000) /* Units: ps */
#define tCCD 4 /* Units: clock cycles */
#define tRP 13750 /* Units: ps */

#define ps2clocks(T) DIV_ROUND_UP((T), tCK)

void dump_mem_params(const ddr3_t *ddr3_mem)
{
	printf("wr2pre : %d\n", ddr3_mem->dram_tmg_0.wr2pre);
	printf("tFAW : %d\n", ddr3_mem->dram_tmg_0.t_faw);
	printf("tRAS_max : %d\n", ddr3_mem->dram_tmg_0.t_ras_max);
	printf("tRAS_min : %d\n", ddr3_mem->dram_tmg_0.t_ras_min);
	printf("tRC : %d\n", ddr3_mem->dram_tmg_0.trc);
	printf("tXP : %d\n", ddr3_mem->dram_tmg_1.t_xp);
	printf("rd2pre : %d\n", ddr3_mem->dram_tmg_1.rd2pre);
	printf("rd2wr : %d\n", ddr3_mem->dram_tmg_1.rd2wr);
	printf("wr2rd : %d\n", ddr3_mem->dram_tmg_1.wr2rd);
	printf("tRCD : %d\n", ddr3_mem->dram_tmg_2.t_rcd);
	printf("tRRD : %d\n", ddr3_mem->dram_tmg_2.t_rrd);
	printf("tCCD : %d\n", ddr3_mem->dram_tmg_2.t_ccd);
	printf("tRP : %d\n", ddr3_mem->dram_tmg_2.t_rp);
	printf("tRFC : %d\n", ddr3_mem->refr_cnt.t_rfc_min);
	printf("tWTR : %d\n", ddr3_mem->misc2.dtpr_twtr);
	printf("tRTP : %d\n", ddr3_mem->misc2.dtpr_trtp);
}

int board_init(void)
{
	return 0;
}

void set_nt5cb256m8fndi_cfg(ddr3_t *ddr3_mem)
{
	ddr3_mem->misc0.full_reg = 0;
	ddr3_mem->misc0.burst_rdwr = 4;
	ddr3_mem->misc0.ddr3_type = 1;
	ddr3_mem->misc0.lpddr2_type = 0;
	ddr3_mem->misc0.active_ranks = 1;
	ddr3_mem->misc0.port_en_0 = 1;
	ddr3_mem->misc0.port_en_1 = 1;
	ddr3_mem->misc0.port_en_2 = 1;

	ddr3_mem->dram_tmg_0.full_reg = 0;
	ddr3_mem->dram_tmg_0.wr2pre = cwl + (bl / 2) + ps2clocks(tWR);
	ddr3_mem->dram_tmg_0.t_faw = ps2clocks(tFAW);
	ddr3_mem->dram_tmg_0.t_ras_max = tRAS_max / (tCK * 1024);
	ddr3_mem->dram_tmg_0.t_ras_min = ps2clocks(tRAS_min);
	ddr3_mem->dram_tmg_0.trc = ps2clocks(tRC);

	ddr3_mem->dram_tmg_1.full_reg = 0;
	ddr3_mem->dram_tmg_1.t_xp = ps2clocks(tXPDLL);
	ddr3_mem->dram_tmg_1.rd2pre = tAL + ps2clocks(tRTP);
	ddr3_mem->dram_tmg_1.read_latency = cl;
	ddr3_mem->dram_tmg_1.rd2wr = cl + (bl / 2) + 2 - cwl;
	ddr3_mem->dram_tmg_1.wr2rd = cwl + (bl / 2) + ps2clocks(tWTR);

	ddr3_mem->dram_tmg_2.full_reg = 0;
	ddr3_mem->dram_tmg_2.write_latency = cwl;
	ddr3_mem->dram_tmg_2.t_rcd = ps2clocks(tRCD) - tAL;
	ddr3_mem->dram_tmg_2.t_rrd = ps2clocks(tRRD);
	ddr3_mem->dram_tmg_2.t_ccd = tCCD;
	ddr3_mem->dram_tmg_2.t_rp = ps2clocks(tRP);

	ddr3_mem->refr_cnt.full_reg = 0;
	ddr3_mem->refr_cnt.t_rfc_min = ps2clocks(160000);

	ddr3_mem->dfi_tmg.full_reg = 0;
	ddr3_mem->dfi_tmg.dfi_t_ctrl_delay = 2;
	ddr3_mem->dfi_tmg.dfi_tphy_wrlat = cwl - 1;
	ddr3_mem->dfi_tmg.dfi_t_rddata_en = cl - 2;
	ddr3_mem->dfi_tmg.dfi_tphy_wrdata = 1;

	ddr3_mem->addrmap0.full_reg = 0;
	ddr3_mem->addrmap0.addrmap_bank_b0 = 7;
	ddr3_mem->addrmap0.addrmap_bank_b1 = 7;
	ddr3_mem->addrmap0.addrmap_bank_b2 = 7;
	ddr3_mem->addrmap0.addrmap_col_b2 = 0;
	ddr3_mem->addrmap0.addrmap_col_b3 = 0;
	ddr3_mem->addrmap0.addrmap_col_b4 = 0;
	ddr3_mem->addrmap0.addrmap_col_b5 = 0;

	ddr3_mem->addrmap1.full_reg = 0;
	ddr3_mem->addrmap1.addrmap_col_b6 = 0;
	ddr3_mem->addrmap1.addrmap_cs_bit0 = 31;
	ddr3_mem->addrmap1.addrmap_col_b7 = 0;
	ddr3_mem->addrmap1.addrmap_col_b8 = 0;
	ddr3_mem->addrmap1.addrmap_col_b9 = 15;
	ddr3_mem->addrmap1.addrmap_col_b10 = 15;
	ddr3_mem->addrmap1.addrmap_col_b11 = 15;

	ddr3_mem->addrmap2.full_reg = 0;
	ddr3_mem->addrmap2.addrmap_row_b0 = 6;
	ddr3_mem->addrmap2.addrmap_row_b1 = 6;
	ddr3_mem->addrmap2.addrmap_row_b2_10 = 6;
	ddr3_mem->addrmap2.addrmap_row_b11 = 6;
	ddr3_mem->addrmap2.addrmap_row_b12 = 6;
	ddr3_mem->addrmap2.addrmap_row_b13 = 6;
	ddr3_mem->addrmap2.addrmap_row_b14 = 6;
	ddr3_mem->addrmap2.addrmap_row_b15 = 15;

	ddr3_mem->misc1.full_reg = 0;

	ddr3_mem->misc2.full_reg = 0;
	ddr3_mem->misc2.dtpr_twtr = ps2clocks(tWTR);
	ddr3_mem->misc2.dtpr_trtp = ps2clocks(tRTP);
	ddr3_mem->misc2.dtpr_trtw = 0;

	ddr3_mem->misc3.full_reg = 0;

	ddr3_mem->misc4.full_reg = 0;
	ddr3_mem->misc4.sel_cpll = CPLL_VALUE;
	ddr3_mem->misc4.ddr_div = 0;
	ddr3_mem->misc4.pgcr_ranken = 1;
	ddr3_mem->misc4.pgcr_rfshdt = 1;
	ddr3_mem->misc4.ddr_remap = 0;

	ddr3_mem->misc5.full_reg = 0;
	ddr3_mem->misc5.dfi_upd_int_min = 12;
	ddr3_mem->misc5.dfi_upd_int_max = 64;

	ddr3_mem->config_0.full_reg = 0;
	ddr3_mem->config_0.phy_mr_cl = cl - 4;
	ddr3_mem->config_0.phy_mr_bl = 0;
	ddr3_mem->config_0.phy_mr_wr = ps2clocks(tWR) - 4;
	ddr3_mem->config_0.phy_mr_cwl = cwl - 5;

	ddr3_mem->config_1.full_reg = 0;
	ddr3_mem->config_1.dtpr_tmrd = 0;
	ddr3_mem->config_1.dtpr_tmod = 0;
}

static bool is_ddrmc_active(int ddrmc_id)
{
	cmctr_t *CMCTR = (cmctr_t *)CMCTR_BASE;
	ddrmc_t *DDRMC;
	bool clk_en;

	uint32_t reg = CMCTR->GATE_CORE_CTR;

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

void dram_init_banksize(void)
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
}

#ifdef CONFIG_SPL_BUILD
static int ddr3_init(ddr3_t *ddr3_mem)
{
	int i;
	uint32_t ret = 0;
	cmctr_t *CMCTR = (cmctr_t *)CMCTR_BASE;
	pmctr_t *PMCTR = (pmctr_t *)PMCTR_BASE;
	ddrmc_t *DDRMC[2] = { (ddrmc_t *)DDRMC0_BASE, (ddrmc_t *)DDRMC1_BASE };
	ddrphy_t *DDRPHY[2] = { (ddrphy_t *)DDRPHY0_BASE,
				(ddrphy_t *)DDRPHY1_BASE };

	if (!ddr3_mem)
		return 1;

	while ((PMCTR->CORE_PWR_STATUS & 1))
		continue;

	for (i = 0; i < 2; i++) {
		CMCTR->GATE_CORE_CTR |= (1 << (i + 1));
		/* Put ACDLL in reset mode as soon as possible since input
		 * clock frequency is too low. Workaround for bug rf#1963.
		 */
		DDRPHY[i]->ACDLLCR = 0;
		DDRMC[i]->DFIMISC = 0;
		bootrom_umctl2_load(ddr3_mem, (void *)DDRMC[i]);

		DDRMC[i]->RFSHTMG = (DDRMC[i]->RFSHTMG & ~0x1ff) |
				ddr3_mem->refr_cnt.t_rfc_min;
	}

	PMCTR->DDR_INIT_END = 0x1;

	/* Program DDR PUB registers */
	for (i = 0; i < 2; i++) {
		bootrom_ddr3_pub_load(ddr3_mem, (void *)DDRPHY[i]);

		/* t_ras_min parameter is not set in bootrom_ddr3_pub_load().
		 * Set it here.
		 */
		DDRPHY[i]->DTPR0 = (DDRPHY[i]->DTPR0 & ~0x1f0000) |
				(ddr3_mem->dram_tmg_0.t_ras_min << 16);

		/* tXS, tXP, tCKE, tDLLK timing parameters are not set in
		 * bootrom_ddr3_pub_load(). Set it here.
		 */
		DDRPHY[i]->DTPR2 = (512 | (10 << 10) | (4 << 15) |
				    (512 << 19));
	}

	CMCTR->SEL_CPLL = CPLL_VALUE;
	while ((CMCTR->SEL_CPLL & 0x80000000) == 0)
		continue;

	/* Wait for a minimum reset duration time. */
	udelay(1);

	for (i = 0; i < 2; i++) {
		/* Put ACDLL out of reset */
		/* TODO: Describe bits */
		DDRPHY[i]->ACDLLCR = 0x40000000;

		/* Wait for DLL lock */
		udelay(10);
		bootrom_init_start(DDRPHY[i], 1);
	}

	/* Wait for DDR PHY initialization complete and then enable DDRMC */
	for (i = 0; i < 2; i++) {
		if (bootrom_pub_init_cmpl_wait((void *)DDRPHY[i], 1, 0)) {
			ret |= (2 << (i * 16));
		} else {
			DDRMC[i]->INIT0 = (1 << 31);
			DDRMC[i]->DFIMISC = 1;
			bootrom_umctl2_norm_wait((void *)DDRMC[i]);
			DDRMC[i]->PCTRL_0 = ddr3_mem->misc0.port_en_0;
			DDRMC[i]->PCTRL1 = ddr3_mem->misc0.port_en_1;
			DDRMC[i]->PCTRL2 = ddr3_mem->misc0.port_en_2;
		}
	}

	return ret;
}
#endif

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_0_SIZE;
#ifdef CONFIG_SPL_BUILD
	ddr3_t ddr3_mem;

	puts("DDR controllers init started\n");
	set_nt5cb256m8fndi_cfg(&ddr3_mem);

	timer_init();

	uint32_t status = ddr3_init(&ddr3_mem);
	if (status & 0xffff)
		puts("DDR controller #0 init failed\n");
	else
		puts("DDR controller #0 init done\n");

	if ((status >> 16) & 0xffff)
		puts("DDR controller #1 init failed\n");
	else
		puts("DDR controller #1 init done\n");

	return status;
#else
	return 0;
#endif
}

#ifdef CONFIG_GENERIC_MMC
int board_mmc_init(bd_t *bis)
{
	return sdmmc_init(SDMMC0_BASE, SPLL_FREQ);
}
#endif

#ifdef CONFIG_SPL_BUILD
u32 spl_boot_device(void)
{
	smctr_t *SMCTR = (smctr_t *)SMCTR_BASE;

	switch (SMCTR->BOOT) {
	case SMCTR_BOOT_SPI0:
		return BOOT_DEVICE_SPI;
	case SMCTR_BOOT_SDMMC0:
		return BOOT_DEVICE_MMC1;
	}

	return BOOT_DEVICE_NONE;
}

u32 spl_boot_mode(void)
{
	return MMCSD_MODE_RAW;
}
#endif
