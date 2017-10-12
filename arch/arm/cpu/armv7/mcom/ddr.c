/*
 * Copyright 2016 ELVEES NeoTek JSC
 * Copyright 2017 RnD Center "ELVEES", JSC
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <asm/arch/bitfield-ops.h>
#include <asm/arch/ddr.h>
#include <asm/arch/reg-fields-ddr.h>
#include <asm/arch/regs.h>
#include <common.h>

#define REFRESH_NUM_DUR_TRAIN       1

/* The following constants are DDRMC internal values used to create address
 * mapping of HIF to SDRAM addresses. See DDRMC ADDRMAPx register definition
 * from 1892VM14YA user manual for more details.
 */
#define COLUMN_ADDR_BIT_BASE        2
#define BANK_ADDR_BIT_BASE          2
#define ROW_ADDR_BIT_BASE           6
#define CS_ADDR_BIT_BASE            6
#define BIT_UNUSED_VALUE            15
#define BIT_UNUSED_VALUE_CS         31
#define COLUMN_ADDR_BITS_MIN        6
#define COLUMN_ADDR_BITS_MAX        12
#define BANK_ADDR_BITS_MAX          3
#define ROW_ADDR_BITS_MAX           16
#define ROW_ADDR_BITS_MIN           11

/* The following constants are DDR PHY timing values used during PHY and SDRAM
 * initialization. See DDR PHY PTRx register definition from 1892VM14YA user
 * manual for more details.
 */
#define DLL_SRST_TIME_PS            50000
#define DLL_LOCK_TIME_PS            5120000
#define ITM_SRST_TIME_CK            8
#define TDINIT0_DDR3_PS             500000000
#define TDINIT2_DDR3_PS             200000000
#define TDINIT0_LPDDR2_PS           200000000
#define TDINIT1_LPDDR2_PS           100000
#define TDINIT2_LPDDR2_PS           11000000
#define TDINIT3_LPDDR2_PS           1000000

static cmctr_t *const CMCTR = (cmctr_t *)CMCTR_BASE;
static pmctr_t *const PMCTR = (pmctr_t *)PMCTR_BASE;
static ddrmc_t *const DDRMC[2] = { (ddrmc_t *)DDRMC0_BASE,
				   (ddrmc_t *)DDRMC1_BASE };
static ddrphy_t *const DDRPHY[2] = { (ddrphy_t *)DDRPHY0_BASE,
				     (ddrphy_t *)DDRPHY1_BASE };

struct ctl_address_mapping {
	u8 column[COLUMN_ADDR_BITS_MAX];
	u8 row[ROW_ADDR_BITS_MAX];
	u8 bank[BANK_ADDR_BITS_MAX];
	u8 cs;
};

u32 ddr_get_clock_period(int ctl_id, struct ddr_freq *freq)
{
	u32 frequency = freq->xti_freq * (freq->cpll_mult + 1);

	switch (ctl_id) {
	case 0:
		frequency = frequency >> freq->ddr0_div;
		break;
	case 1:
		frequency = frequency >> freq->ddr1_div;
		break;
	default:
		printf("DDR Memory Controller ID (%d) is not valid\n", ctl_id);
		return 0;
	}

	return DIV_ROUND_UP(1000000000, frequency / 1000);
}

static int find_first_unused_bit(u8 *data, int size)
{
	int i;

	for (i = 0; i < size; i++)
		if (data[i] == BIT_UNUSED_VALUE)
			return i;

	return size;
}

/* Create mapping of SDRAM column addresses to system addresses on the base of
 * SDRAM parameters.
 */
static u16 create_column_mapping(struct ctl_address_mapping *mapping,
				 struct sdram_params_common *par)
{
	int i;

	if (par->columns < (1 << COLUMN_ADDR_BITS_MIN)) {
		printf("Number of columns (%d) is not supported\n",
		       par->columns);
		return MCOM_DDR_CFG_ERR;
	}

	for (i = COLUMN_ADDR_BITS_MIN; i < COLUMN_ADDR_BITS_MAX; i++) {
		if ((1 << i) >= par->columns) {
			if (par->bl == 4)
				mapping->column[i] = BIT_UNUSED_VALUE;
			if (par->bl == 8)
				mapping->column[i - 1] = BIT_UNUSED_VALUE;
		}
	}

	return 0;
}

/* Create mapping of SDRAM bank addresses to system addresses on the base of
 * SDRAM parameters.
 */
static u16 create_bank_mapping(struct ctl_address_mapping *mapping,
			       struct sdram_params_common *par)
{
	u8 bit;

	bit = find_first_unused_bit(mapping->column,
				    ARRAY_SIZE(mapping->column));

	switch (par->banks) {
	case 4:
		mapping->bank[2] = BIT_UNUSED_VALUE;
		break;
	case 8:
		mapping->bank[2] = bit - BANK_ADDR_BIT_BASE;
		break;
	default:
		printf("Number of banks (%d) is unsupported", par->banks);
		return MCOM_DDR_CFG_ERR;
	}

	mapping->bank[0] = bit - BANK_ADDR_BIT_BASE;
	mapping->bank[1] = bit - BANK_ADDR_BIT_BASE;

	return 0;
}

/* Create mapping of SDRAM row addresses to system addresses on the base of
 * SDRAM parameters.
 */
static u16 create_row_mapping(struct ctl_address_mapping *mapping,
			      struct sdram_params_common *par)
{
	u8 bit;
	int i;

	if (par->rows < (1 << ROW_ADDR_BITS_MIN)) {
		printf("Number of rows (%d) is not supported\n",
		       par->rows);
		return MCOM_DDR_CFG_ERR;
	}

	bit = find_first_unused_bit(mapping->bank,
				    ARRAY_SIZE(mapping->bank));

	bit += mapping->bank[0] + BANK_ADDR_BIT_BASE;

	for (i = 0; i < ROW_ADDR_BITS_MIN; i++)
		mapping->row[i] = bit - ROW_ADDR_BIT_BASE;

	for (i = ROW_ADDR_BITS_MIN; i < ROW_ADDR_BITS_MAX; i++) {
		if ((1 << i) >= par->rows)
			mapping->row[i] = BIT_UNUSED_VALUE;
		else
			mapping->row[i] = bit - ROW_ADDR_BIT_BASE;
	}

	return 0;
}

/* Create mapping of SDRAM rank or cs addresses to system addresses on the base
 * of SDRAM parameters.
 */
static u16 create_cs_mapping(struct ctl_address_mapping *mapping,
			     struct sdram_params_common *par)
{
	u8 bit;

	bit = find_first_unused_bit(mapping->row,
				    ARRAY_SIZE(mapping->row));

	bit += mapping->row[0] + ROW_ADDR_BIT_BASE;

	switch (par->ranks) {
	case MCOM_SDRAM_ONE_RANK:
		mapping->cs = BIT_UNUSED_VALUE_CS;
		break;
	case MCOM_SDRAM_TWO_RANKS:
		mapping->cs = bit - CS_ADDR_BIT_BASE;
		break;
	default:
		printf("Number of ranks (%d) is unsupported", par->ranks);
		return MCOM_DDR_CFG_ERR;
	}

	return 0;
}

static u16 set_addr_mapping(struct ddr_cfg *cfg)
{
	struct ctl_address_mapping mapping;
	ddrmc_t *const MC = DDRMC[cfg->ctl_id];
	u16 ret;

	memset(&mapping, 0, sizeof(mapping));

	ret = create_column_mapping(&mapping, &cfg->common);
	if (ret)
		return ret;

	ret = create_bank_mapping(&mapping, &cfg->common);
	if (ret)
		return ret;

	ret = create_row_mapping(&mapping, &cfg->common);
	if (ret)
		return ret;

	ret = create_cs_mapping(&mapping, &cfg->common);
	if (ret)
		return ret;

	MC->ADDRMAP0 = FIELD_PREP(ADDRMAP0_CS_BIT0, mapping.cs);

	MC->ADDRMAP1 = FIELD_PREP(ADDRMAP1_BANK_BIT0, mapping.bank[0]) |
		       FIELD_PREP(ADDRMAP1_BANK_BIT1, mapping.bank[1]) |
		       FIELD_PREP(ADDRMAP1_BANK_BIT2, mapping.bank[2]);

	MC->ADDRMAP2 = FIELD_PREP(ADDRMAP2_COL_BIT2, mapping.column[2]) |
		       FIELD_PREP(ADDRMAP2_COL_BIT3, mapping.column[3]) |
		       FIELD_PREP(ADDRMAP2_COL_BIT4, mapping.column[4]) |
		       FIELD_PREP(ADDRMAP2_COL_BIT5, mapping.column[5]);

	MC->ADDRMAP3 = FIELD_PREP(ADDRMAP3_COL_BIT6, mapping.column[6]) |
		       FIELD_PREP(ADDRMAP3_COL_BIT7, mapping.column[7]) |
		       FIELD_PREP(ADDRMAP3_COL_BIT8, mapping.column[8]) |
		       FIELD_PREP(ADDRMAP3_COL_BIT9, mapping.column[9]);

	MC->ADDRMAP4 = FIELD_PREP(ADDRMAP4_COL_BIT10, mapping.column[10]) |
		       FIELD_PREP(ADDRMAP4_COL_BIT11, mapping.column[11]);

	MC->ADDRMAP5 = FIELD_PREP(ADDRMAP5_ROW_BIT0, mapping.row[0]) |
		       FIELD_PREP(ADDRMAP5_ROW_BIT1, mapping.row[1]) |
		       FIELD_PREP(ADDRMAP5_ROW_BIT2_10, mapping.row[2]) |
		       FIELD_PREP(ADDRMAP5_ROW_BIT11, mapping.row[11]);

	MC->ADDRMAP6 = FIELD_PREP(ADDRMAP6_ROW_BIT12, mapping.row[12]) |
		       FIELD_PREP(ADDRMAP6_ROW_BIT13, mapping.row[13]) |
		       FIELD_PREP(ADDRMAP6_ROW_BIT14, mapping.row[14]) |
		       FIELD_PREP(ADDRMAP6_ROW_BIT15, mapping.row[15]);

	return 0;
}

static u16 ctl_set_regs_ddr3(struct ddr_cfg *cfg)
{
	u32 tmp, tmp2;
	ddrmc_t *const MC = DDRMC[cfg->ctl_id];

	/* TODO: Calculate the following parameters from anywhere */
	MC->DFIUPD1 = FIELD_PREP(DFIUPD1_UPD_INTER_MAX, 64) |
		      FIELD_PREP(DFIUPD1_UPD_INTER_MIN, 12);

	MC->MSTR = FIELD_PREP(MSTR_DDR3, 1) |
		   FIELD_PREP(MSTR_BURST_RDWR, cfg->common.bl >> 1) |
		   FIELD_PREP(MSTR_ACTIVE_RANKS, cfg->common.ranks);

	MC->RFSHTMG = FIELD_PREP(RFSHTMG_TRFC_MIN, cfg->ddr3.trfc) |
		      FIELD_PREP(RFSHTMG_TRFC_NOM, cfg->common.trefi / 32);

	tmp = cfg->common.tras_max / 1024;
	tmp2 = cfg->common.cwl + cfg->common.twr + cfg->common.bl / 2;
	MC->DRAMTMG0 = FIELD_PREP(DRAMTMG0_TRAS_MIN, cfg->common.tras) |
		       FIELD_PREP(DRAMTMG0_TRAS_MAX, tmp) |
		       FIELD_PREP(DRAMTMG0_TFAW, cfg->common.tfaw) |
		       FIELD_PREP(DRAMTMG0_WR2PRE, tmp2);

	tmp = max(cfg->common.trtp, (u32)4);
	MC->DRAMTMG1 = FIELD_PREP(DRAMTMG1_TRC, cfg->common.trc) |
		       FIELD_PREP(DRAMTMG1_RD2PRE, tmp) |
		       FIELD_PREP(DRAMTMG1_TXP, cfg->ddr3.txpdll);

	tmp = cfg->common.cwl + cfg->common.bl / 2 + cfg->common.twtr;
	tmp2 = cfg->common.cl + cfg->common.bl / 2 + 2 - cfg->common.cwl;
	MC->DRAMTMG2 = FIELD_PREP(DRAMTMG2_WR2RD, tmp) |
		       FIELD_PREP(DRAMTMG2_RD2WR, tmp2) |
		       FIELD_PREP(DRAMTMG2_READ_LAT, cfg->common.cl) |
		       FIELD_PREP(DRAMTMG2_WRITE_LAT, cfg->common.cwl);

	MC->DRAMTMG3 = FIELD_PREP(DRAMTMG3_TMOD, cfg->ddr3.tmod) |
		       FIELD_PREP(DRAMTMG3_TMRD, cfg->ddr3.tmrd);

	MC->DRAMTMG4 = FIELD_PREP(DRAMTMG4_TRP, cfg->ddr3.trp) |
		       FIELD_PREP(DRAMTMG4_TRRD, cfg->common.trrd) |
		       FIELD_PREP(DRAMTMG4_TCCD, cfg->common.tccd) |
		       FIELD_PREP(DRAMTMG4_TRCD, cfg->common.trcd);

	MC->DRAMTMG5 = FIELD_PREP(DRAMTMG5_TCKE, cfg->common.tcke) |
		       FIELD_PREP(DRAMTMG5_TCKESR, cfg->common.tckesr) |
		       FIELD_PREP(DRAMTMG5_TCKSRE, cfg->ddr3.tcksre) |
		       FIELD_PREP(DRAMTMG5_TCKSRX, cfg->ddr3.tcksrx);

	MC->ZQCTL0 = FIELD_PREP(ZQCTL0_TZQSHORT, cfg->common.tzqcs) |
		     FIELD_PREP(ZQCTL0_TZQLONG, cfg->ddr3.tzqoper);

	MC->DFITMG0 = FIELD_PREP(DFITMG0_TPHY_WRLAT, cfg->common.cwl - 1) |
		      FIELD_PREP(DFITMG0_TPHY_WRDATA, 1) |
		      FIELD_PREP(DFITMG0_RDDATA_EN, cfg->common.cl - 2) |
		      FIELD_PREP(DFITMG0_CTRL_DELAY, 2);

	MC->DFITMG1 = FIELD_PREP(DFITMG1_DRAM_CLK_EN, 2) |
		      FIELD_PREP(DFITMG1_DRAM_CLK_DIS, 2) |
		      FIELD_PREP(DFITMG1_WRDATA_DELAY, cfg->common.cwl);

	return set_addr_mapping(cfg);
}

static u16 ctl_set_regs_lpddr2(struct ddr_cfg *cfg, struct ddr_freq *freq)
{
	u32 tck, tmp, tmp2;
	ddrmc_t *const MC = DDRMC[cfg->ctl_id];

	tck = ddr_get_clock_period(cfg->ctl_id, freq);
	if (!tck)
		return MCOM_DDR_CFG_ERR;

	/* TODO: Calculate the following parameters from anywhere */
	MC->DFIUPD1 = FIELD_PREP(DFIUPD1_UPD_INTER_MAX, 64) |
		      FIELD_PREP(DFIUPD1_UPD_INTER_MIN, 12);

	MC->MSTR = FIELD_PREP(MSTR_LPDDR2, 1) |
		   FIELD_PREP(MSTR_BURST_RDWR, cfg->common.bl / 2) |
		   FIELD_PREP(MSTR_ACTIVE_RANKS, cfg->common.ranks);

	MC->RFSHTMG = FIELD_PREP(RFSHTMG_TRFC_MIN, cfg->lpddr2.trfcab) |
		      FIELD_PREP(RFSHTMG_TRFC_NOM, cfg->common.trefi / 32);

	tmp = cfg->common.tras_max / 1024;
	tmp2 = cfg->common.cwl + cfg->common.twr + cfg->common.bl / 2 + 1;
	MC->DRAMTMG0 = FIELD_PREP(DRAMTMG0_TRAS_MIN, cfg->common.tras) |
		       FIELD_PREP(DRAMTMG0_TRAS_MAX, tmp) |
		       FIELD_PREP(DRAMTMG0_TFAW, cfg->common.tfaw) |
		       FIELD_PREP(DRAMTMG0_WR2PRE, tmp2);

	switch (cfg->lpddr2.device_type) {
	case MCOM_LPDDR2_TYPE_S2:
		tmp = cfg->common.bl / 2 + cfg->common.trtp - 1;
		break;
	case MCOM_LPDDR2_TYPE_S4:
		tmp = cfg->common.bl / 2 + max(cfg->common.trtp, (u32)2) - 2;
		break;
	default:
		printf("LPDDR2 device type (%d) is not supported\n",
		       cfg->lpddr2.device_type);
		return MCOM_DDR_CFG_ERR;
	}

	MC->DRAMTMG1 = FIELD_PREP(DRAMTMG1_TRC, cfg->common.trc) |
		       FIELD_PREP(DRAMTMG1_RD2PRE, tmp) |
		       FIELD_PREP(DRAMTMG1_TXP, cfg->common.txp);

	tmp = cfg->common.cwl + cfg->common.bl / 2 + cfg->common.twtr + 1;
	tmp2 = cfg->common.cl + cfg->common.bl / 2 + 1 - cfg->common.cwl
		+ to_clocks(cfg->lpddr2.tdqsck_max, tck);
	MC->DRAMTMG2 = FIELD_PREP(DRAMTMG2_WR2RD, tmp) |
		       FIELD_PREP(DRAMTMG2_RD2WR, tmp2) |
		       FIELD_PREP(DRAMTMG2_READ_LAT, cfg->common.cl) |
		       FIELD_PREP(DRAMTMG2_WRITE_LAT, cfg->common.cwl);

	MC->DRAMTMG3 = FIELD_PREP(DRAMTMG3_TMRW, cfg->lpddr2.tmrw);

	tmp = max(cfg->lpddr2.trpab, cfg->lpddr2.trppb);
	MC->DRAMTMG4 = FIELD_PREP(DRAMTMG4_TRP, tmp) |
		       FIELD_PREP(DRAMTMG4_TRRD, cfg->common.trrd) |
		       FIELD_PREP(DRAMTMG4_TCCD, cfg->common.tccd) |
		       FIELD_PREP(DRAMTMG4_TRCD, cfg->common.trcd);

	tmp = max(cfg->common.tcke, cfg->common.tckesr);
	MC->DRAMTMG5 = FIELD_PREP(DRAMTMG5_TCKE, tmp) |
		       FIELD_PREP(DRAMTMG5_TCKESR, cfg->common.tckesr) |
		       FIELD_PREP(DRAMTMG5_TCKSRE, 2) |
		       FIELD_PREP(DRAMTMG5_TCKSRX, 2);

	tmp = MC->DRAMTMG6 & ~DRAMTMG6_TCKCSX;
	MC->DRAMTMG6 = tmp | FIELD_PREP(DRAMTMG6_TCKCSX, cfg->common.txp + 2);

	MC->ZQCTL0 = FIELD_PREP(ZQCTL0_TZQSHORT, cfg->common.tzqcs) |
		     FIELD_PREP(ZQCTL0_TZQLONG, cfg->lpddr2.tzqcl);

	tmp = MC->ZQCTL1 & ~ZQCTL1_TZQRESET;
	MC->ZQCTL1 = tmp | FIELD_PREP(ZQCTL1_TZQRESET, cfg->lpddr2.tzqreset);

	tmp = cfg->common.cl - 2 + to_clocks(cfg->lpddr2.tdqsck, tck);
	MC->DFITMG0 = FIELD_PREP(DFITMG0_TPHY_WRLAT, cfg->common.cwl) |
		      FIELD_PREP(DFITMG0_TPHY_WRDATA, 1) |
		      FIELD_PREP(DFITMG0_RDDATA_EN, tmp) |
		      FIELD_PREP(DFITMG0_CTRL_DELAY, 2);

	MC->DFITMG1 = FIELD_PREP(DFITMG1_DRAM_CLK_EN, 2) |
		      FIELD_PREP(DFITMG1_DRAM_CLK_DIS, 2) |
		      FIELD_PREP(DFITMG1_WRDATA_DELAY, cfg->common.cwl + 1);

	return set_addr_mapping(cfg);
}

static u16 set_impedance_ddr3(struct impedance_params *impedance, ddrphy_t *PHY)
{
	u32 tmp = 0;
	bool odt_en = true;

	switch (impedance->ods_dram) {
	case 34:
		tmp = BIT(1);
		break;
	case 40:
		break;
	default:
		printf("ods_dram = %d is not supported\n", impedance->ods_dram);
		return MCOM_DDR_CFG_ERR;
	}

	switch (impedance->odt_dram) {
	case 0:
		break;
	case 40:
		tmp |= BIT(2) | BIT(6);
		break;
	case 60:
		tmp |= BIT(2);
		break;
	case 120:
		tmp |= BIT(6);
		break;
	default:
		printf("odt_dram = %d is not supported\n", impedance->odt_dram);
		return MCOM_DDR_CFG_ERR;
	}

	PHY->MR1 = tmp;

	switch (impedance->ods_mc) {
	case 34:
		tmp = 0xC;
		break;
	case 40:
		tmp = 0xB;
		break;
	default:
		printf("ods_mc = %d is not supported\n", impedance->ods_mc);
		return MCOM_DDR_CFG_ERR;
	}

	switch (impedance->odt_mc) {
	case 0:
		odt_en = false;
		break;
	case 40:
		tmp |= (8 << 4);
		break;
	case 60:
		tmp |= (5 << 4);
		break;
	case 120:
		tmp |= (1 << 4);
		break;
	default:
		printf("odt_mc = %d is not supported\n", impedance->odt_mc);
		return MCOM_DDR_CFG_ERR;
	}

	PHY->ZQ0CR1 = tmp;

	if (odt_en)
		PHY->DXCCR |= 1;

	return 0;
}

static u16 phy_set_regs_ddr3(struct ddr_cfg *cfg, struct ddr_freq *freq)
{
	u32 tck, tmp, tmp2;
	ddrphy_t *const PHY = DDRPHY[cfg->ctl_id];
	u16 ret;

	tck = ddr_get_clock_period(cfg->ctl_id, freq);
	if (!tck)
		return MCOM_DDR_CFG_ERR;

	PHY->PGCR &= ~PGCR_RANKEN;
	PHY->PGCR |= FIELD_PREP(PGCR_RANKEN, cfg->common.ranks) |
		     FIELD_PREP(PGCR_RFSHDT, REFRESH_NUM_DUR_TRAIN);

	PHY->PTR0 = FIELD_PREP(PTR0_TDLLSRST,
			       to_clocks(DLL_SRST_TIME_PS, tck)) |
		    FIELD_PREP(PTR0_TDLLLOCK,
			       to_clocks(DLL_LOCK_TIME_PS, tck)) |
		    FIELD_PREP(PTR0_TITMSRST, ITM_SRST_TIME_CK);

	tmp = cfg->ddr3.trfc + to_clocks(10000, tck);
	PHY->PTR1 = FIELD_PREP(PTR1_TDINIT0, to_clocks(TDINIT0_DDR3_PS, tck)) |
		    FIELD_PREP(PTR1_TDINIT1, tmp);

	PHY->PTR2 = FIELD_PREP(PTR2_TDINIT2, to_clocks(TDINIT2_DDR3_PS, tck));

	PHY->DCR = FIELD_PREP(DCR_DDRMD, 3) |
		   FIELD_PREP(DCR_DDR8BANK, (cfg->common.banks == 8) ? 1 : 0);

	tmp = cfg->ddr3.tmrd;
	PHY->DTPR0 = FIELD_PREP(DTPR0_TMRD, (tmp < 4) ? 0 : tmp - 4) |
		     FIELD_PREP(DTPR0_TRTP, cfg->common.trtp) |
		     FIELD_PREP(DTPR0_TWTR, cfg->common.twtr) |
		     FIELD_PREP(DTPR0_TRP, cfg->ddr3.trp) |
		     FIELD_PREP(DTPR0_TRCD, cfg->common.trcd) |
		     FIELD_PREP(DTPR0_TRAS, cfg->common.tras) |
		     FIELD_PREP(DTPR0_TRRD, cfg->common.trrd) |
		     FIELD_PREP(DTPR0_TRC, cfg->common.trc) |
		     FIELD_PREP(DTPR0_TCCD, (cfg->common.tccd <= 4) ? 0 : 1);

	tmp = cfg->ddr3.tmod;
	PHY->DTPR1 = FIELD_PREP(DTPR1_TFAW, cfg->common.tfaw) |
		     FIELD_PREP(DTPR1_TMOD, (tmp < 12) ? 0 : tmp - 12) |
		     FIELD_PREP(DTPR1_TRFC, cfg->ddr3.trfc);

	tmp = max(cfg->ddr3.txs, cfg->ddr3.txsdll);
	PHY->DTPR2 = FIELD_PREP(DTPR2_TXS, tmp) |
		     FIELD_PREP(DTPR2_TXP,
				max(cfg->common.txp, cfg->ddr3.txpdll)) |
		     FIELD_PREP(DTPR2_TCKE, cfg->common.tckesr) |
		     FIELD_PREP(DTPR2_TDLLK, cfg->ddr3.tdllk);

	switch (cfg->common.bl) {
	case 4:
		tmp = FIELD_PREP(MR0_DDR3_BL, 2);
		break;
	case 8:
		tmp = FIELD_PREP(MR0_DDR3_BL, 0);
		break;
	default:
		printf("Burst length (%d) is not supported\n", cfg->common.bl);
		return MCOM_DDR_CFG_ERR;
	}

	if ((cfg->common.twr >= MCOM_DDR_MIN_TWR) && (cfg->common.twr <= 8)) {
		tmp2 = cfg->common.twr - 4;
	} else if ((cfg->common.twr == 10) || (cfg->common.twr == 12)) {
		tmp2 = cfg->common.twr >> 1;
	} else {
		printf("tWR (%d) is not supported\n", cfg->common.twr);
		return MCOM_DDR_CFG_ERR;
	}

	PHY->MR0 = FIELD_PREP(MR0_DDR3_BL, tmp) |
		   FIELD_PREP(MR0_DDR3_CL, cfg->common.cl - 4) |
		   FIELD_PREP(MR0_DDR3_WR, tmp2);

	PHY->MR2 = FIELD_PREP(MR2_DDR3_CWL, cfg->common.cwl - 5);

	ret = set_impedance_ddr3(&cfg->impedance, PHY);
	if (ret)
		return ret;

	return 0;
}

static u16 set_impedance_lpddr2(struct impedance_params *impedance,
				ddrphy_t *PHY)
{
	u32 tmp;

	switch (impedance->ods_dram) {
	case 34:
		tmp = 1;
		break;
	case 40:
		tmp = 2;
		break;
	case 48:
		tmp = 3;
		break;
	case 60:
		tmp = 4;
		break;
	case 80:
		tmp = 6;
		break;
	case 120:
		tmp = 7;
		break;
	default:
		printf("ods_dram = %d is not supported\n", impedance->ods_dram);
		return MCOM_DDR_CFG_ERR;
	}
	PHY->MR3 = tmp;

	switch (impedance->ods_mc) {
	case 34:
		tmp = 13;
		break;
	case 40:
		tmp = 11;
		break;
	case 48:
		tmp = 9;
		break;
	case 60:
		tmp = 7;
		break;
	case 80:
		tmp = 5;
		break;
	default:
		printf("ods_mc = %d is not supported\n", impedance->ods_mc);
		return MCOM_DDR_CFG_ERR;
	}
	PHY->ZQ0CR1 = tmp;

	return 0;
}

static u16 phy_set_regs_lpddr2(struct ddr_cfg *cfg, struct ddr_freq *freq)
{
	u32 tck, tmp, tmp2;
	ddrphy_t *const PHY = DDRPHY[cfg->ctl_id];
	u16 ret;

	tck = ddr_get_clock_period(cfg->ctl_id, freq);
	if (!tck)
		return MCOM_DDR_CFG_ERR;

	PHY->PGCR &= ~(PGCR_DFTCMP | PGCR_RANKEN);
	PHY->PGCR |= FIELD_PREP(PGCR_DQSCFG, 1) |
		     FIELD_PREP(PGCR_RANKEN, cfg->common.ranks) |
		     FIELD_PREP(PGCR_RFSHDT, REFRESH_NUM_DUR_TRAIN);

	PHY->PTR0 = FIELD_PREP(PTR0_TDLLSRST,
			       to_clocks(DLL_SRST_TIME_PS, tck)) |
		    FIELD_PREP(PTR0_TDLLLOCK,
			       to_clocks(DLL_LOCK_TIME_PS, tck)) |
		    FIELD_PREP(PTR0_TITMSRST, ITM_SRST_TIME_CK);

	tmp = to_clocks(TDINIT0_LPDDR2_PS, tck);
	tmp2 = to_clocks(TDINIT1_LPDDR2_PS, tck);
	PHY->PTR1 = FIELD_PREP(PTR1_TDINIT0, tmp) |
		    FIELD_PREP(PTR1_TDINIT1, tmp2);

	tmp = to_clocks(TDINIT2_LPDDR2_PS, tck);
	tmp2 = to_clocks(TDINIT3_LPDDR2_PS, tck);
	PHY->PTR2 = FIELD_PREP(PTR2_TDINIT2, tmp) |
		    FIELD_PREP(PTR2_TDINIT3, tmp2);

	PHY->DXCCR = FIELD_PREP(DXCCR_DQSRES, cfg->ctl.dqsres) |
		     FIELD_PREP(DXCCR_DQSNRES, cfg->ctl.dqsnres);

	tmp = to_clocks(cfg->lpddr2.tdqsck_max - cfg->lpddr2.tdqsck, tck);
	PHY->DSGCR &= ~(DSGCR_DQSGX | DSGCR_DQSGE | DSGCR_NL2OE);
	PHY->DSGCR |= FIELD_PREP(DSGCR_DQSGX, tmp) |
		      FIELD_PREP(DSGCR_DQSGE, tmp) |
		      FIELD_PREP(DSGCR_NL2PD, 1);

	PHY->DCR = FIELD_PREP(DCR_DDRMD, 4) |
		   FIELD_PREP(DCR_DDR8BANK, (cfg->common.banks == 8) ? 1 : 0) |
		   FIELD_PREP(DCR_DDRTYPE, cfg->lpddr2.device_type);

	tmp = max(cfg->lpddr2.trpab, cfg->lpddr2.trppb);
	PHY->DTPR0 = FIELD_PREP(DTPR0_TRTP, cfg->common.trtp) |
		     FIELD_PREP(DTPR0_TWTR, cfg->common.twtr) |
		     FIELD_PREP(DTPR0_TRP, tmp) |
		     FIELD_PREP(DTPR0_TRCD, cfg->common.trcd) |
		     FIELD_PREP(DTPR0_TRAS, cfg->common.tras) |
		     FIELD_PREP(DTPR0_TRRD, cfg->common.trrd) |
		     FIELD_PREP(DTPR0_TRC, cfg->common.trc) |
		     FIELD_PREP(DTPR0_TCCD, (cfg->common.tccd <= 4) ? 0 : 1);

	tmp = to_clocks(cfg->lpddr2.tdqsck_max, tck);
	PHY->DTPR1 = FIELD_PREP(DTPR1_TFAW, cfg->common.tfaw) |
		     FIELD_PREP(DTPR1_TRFC, cfg->lpddr2.trfcab) |
		     FIELD_PREP(DTPR1_TDQSMIN, cfg->lpddr2.tdqsck / tck) |
		     FIELD_PREP(DTPR1_TDQSMAX, tmp);

	PHY->DTPR2 = FIELD_PREP(DTPR2_TXS, cfg->lpddr2.txsr) |
		     FIELD_PREP(DTPR2_TXP, cfg->common.txp) |
		     FIELD_PREP(DTPR2_TCKE, cfg->common.tckesr);

	if (cfg->common.bl != 4 && cfg->common.bl != 8 &&
	    cfg->common.bl != 16) {
		printf("Burst length (%d) is not supported\n", cfg->common.bl);
		return MCOM_DDR_CFG_ERR;
	}

	PHY->MR1 = FIELD_PREP(MR1_LPDDR2_BL, ffs(cfg->common.bl) - 1);

	if (cfg->common.twr < 3 || cfg->common.twr > 8) {
		printf("tWR (%d) is not supported\n", cfg->common.twr);
		return MCOM_DDR_CFG_ERR;
	}

	PHY->MR1 |= FIELD_PREP(MR1_LPDDR2_NWR, cfg->common.twr - 2);

	if (cfg->common.cl == 3 && cfg->common.cwl == 1) {
		tmp = 1;
	} else if (cfg->common.cl == 4 && cfg->common.cwl == 2) {
		tmp = 2;
	} else if (cfg->common.cl == 5 && cfg->common.cwl == 2) {
		tmp = 3;
	} else if (cfg->common.cl == 6 && cfg->common.cwl == 3) {
		tmp = 4;
	} else if (cfg->common.cl == 7 && cfg->common.cwl == 4) {
		tmp = 5;
	} else if (cfg->common.cl == 8 && cfg->common.cwl == 4) {
		tmp = 6;
	} else {
		printf("CL/CWL (%d/%d) are not supported\n", cfg->common.cl,
		       cfg->common.cwl);
		return MCOM_DDR_CFG_ERR;
	}
	PHY->MR2 = FIELD_PREP(MR2_LPDDR2_RL_WL, tmp);

	ret = set_impedance_lpddr2(&cfg->impedance, PHY);
	if (ret)
		return ret;

	return 0;
}

static void phy_acdll_srst_en(int ctl_id, int srst_en)
{
	DDRPHY[ctl_id]->ACDLLCR = FIELD_PREP(ACDLLCR_DLLSRST,
					     (srst_en) ? 0 : 1);

	/* Wait 10 us to satisfy a minimum reset duration time for PHY ACDLL
	 * or ACDLL locking time
	 */
	udelay(10);
}

static u16 validate_cfg(struct ddr_cfg *cfg)
{
	if ((cfg->ctl_id != 1) && (cfg->ctl_id != 0))
		return MCOM_DDR_CFG_ERR;
	else
		return 0;
}

static u16 ddr_init_phase1(struct ddr_cfg *cfg, struct ddr_freq *freq)
{
	u16 rc = 0;
	int ctl_id = cfg->ctl_id;

	PMCTR->DDR_INIT_END = 0;

	if (PMCTR->DDR_PIN_RET & (1 << ctl_id)) {
		printf("DDR retention should be disabled earlier\n");
		PMCTR->DDR_PIN_RET &= ~(1 << ctl_id);
	}

	CMCTR->GATE_CORE_CTR |= (1 << (ctl_id + 1));
	DDRMC[ctl_id]->DFIMISC = FIELD_PREP(DFIMISC_INIT_COMPLETE_EN, 0);

	switch (cfg->type) {
	case MCOM_SDRAM_TYPE_DDR3:
		rc = ctl_set_regs_ddr3(cfg);
		break;
	case MCOM_SDRAM_TYPE_LPDDR2:
		rc = ctl_set_regs_lpddr2(cfg, freq);
		break;
	default:
		printf("SDRAM type (%d) is unsupported", cfg->type);
		rc = MCOM_DDR_CFG_ERR;
	}

	if (!rc)
		phy_acdll_srst_en(ctl_id, 1);

	return rc;
}

static void ddr_set_freq(struct ddr_freq *freq)
{
	if ((CMCTR->SEL_CPLL & 0xff) != 0) {
		printf("CPLL is set too early\n");
		CMCTR->SEL_CPLL = 0;
	}

	PMCTR->DDR_INIT_END = 0x1;
	CMCTR->DIV_DDR0_CTR = freq->ddr0_div;
	CMCTR->DIV_DDR1_CTR = freq->ddr1_div;

	CMCTR->SEL_CPLL = freq->cpll_mult;
	while ((CMCTR->SEL_CPLL & 0x80000000) == 0)
		continue;
}

static void dqs_gating_override(struct ddr_cfg *cfg)
{
	int ctl_id = cfg->ctl_id;

	/* Disable DQS drift compensation */
	DDRPHY[ctl_id]->PGCR &= ~PGCR_DFTCMP;

	DDRPHY[ctl_id]->DX0DQSTR &= ~DXDQSTR_GATING;
	DDRPHY[ctl_id]->DX0DQSTR |= cfg->ctl.dqs_gating[0];

	DDRPHY[ctl_id]->DX1DQSTR &= ~DXDQSTR_GATING;
	DDRPHY[ctl_id]->DX1DQSTR |= cfg->ctl.dqs_gating[1];

	DDRPHY[ctl_id]->DX2DQSTR &= ~DXDQSTR_GATING;
	DDRPHY[ctl_id]->DX2DQSTR |= cfg->ctl.dqs_gating[2];

	DDRPHY[ctl_id]->DX3DQSTR &= ~DXDQSTR_GATING;
	DDRPHY[ctl_id]->DX3DQSTR |= cfg->ctl.dqs_gating[3];
}

static u16 phy_init(struct ddr_cfg *cfg)
{
	u16 rc = 0;
	u32 tmp;
	int ctl_id = cfg->ctl_id;

	tmp = FIELD_PREP(PIR_INIT, 1) |
	      FIELD_PREP(PIR_DLLSRST, 1) |
	      FIELD_PREP(PIR_DLLLOCK, 1) |
	      FIELD_PREP(PIR_ZCAL, 1) |
	      FIELD_PREP(PIR_ITMSRST, 1) |
	      FIELD_PREP(PIR_DRAMINIT, 1) |
	      FIELD_PREP(PIR_QSTRN, 1) |
	      FIELD_PREP(PIR_RVTRN, 1);

	if (cfg->type == MCOM_SDRAM_TYPE_DDR3)
		tmp |= FIELD_PREP(PIR_DRAMRST, 1);

	/* start PHY and DRAM initialization */
	DDRPHY[ctl_id]->PIR = tmp;

	/* wait for the end of PHY and DRAM initialization */
	while (!FIELD_GET(PGSR_IDONE, DDRPHY[ctl_id]->PGSR))
		continue;

	if (FIELD_GET(PGSR_DTERR, DDRPHY[ctl_id]->PGSR)) {
		rc = MCOM_DDR_TRAIN_ERR;
		printf("DQS gate training error for ctl %d\n", ctl_id);
	}

	if (FIELD_GET(PGSR_RVERR, DDRPHY[ctl_id]->PGSR)) {
		rc = MCOM_DDR_TRAIN_ERR;
		printf("Read valid training error for ctl %d\n", ctl_id);
	}

	if (!rc && cfg->ctl.dqs_gating_override)
		dqs_gating_override(cfg);

	return rc;
}

static void ctl_init(int ctl_id)
{
	DDRMC[ctl_id]->INIT0 = FIELD_PREP(INIT0_SKIP_DRAM_INIT, 1);
	DDRMC[ctl_id]->DFIMISC = FIELD_PREP(DFIMISC_INIT_COMPLETE_EN, 1);

	while (FIELD_GET(STAT_OPER_MODE, DDRMC[ctl_id]->STAT) != 1)
		continue;

	DDRMC[ctl_id]->PCTRL0 = FIELD_PREP(PCTRL_PORT_EN, 1);
	DDRMC[ctl_id]->PCTRL1 = FIELD_PREP(PCTRL_PORT_EN, 1);
	DDRMC[ctl_id]->PCTRL2 = FIELD_PREP(PCTRL_PORT_EN, 1);
}

static u16 ddr_init_phase2(struct ddr_cfg *cfg, struct ddr_freq *freq)
{
	u16 rc = 0;
	int ctl_id = cfg->ctl_id;

	phy_acdll_srst_en(ctl_id, 0);

	switch (cfg->type) {
	case MCOM_SDRAM_TYPE_DDR3:
		rc = phy_set_regs_ddr3(cfg, freq);
		break;
	case MCOM_SDRAM_TYPE_LPDDR2:
		rc = phy_set_regs_lpddr2(cfg, freq);
		break;
	default:
		printf("SDRAM type (%d) is unsupported", cfg->type);
		rc = MCOM_DDR_CFG_ERR;
	}

	if (rc)
		return rc;

	rc = phy_init(cfg);
	if (rc)
		return rc;

	ctl_init(ctl_id);

	return MCOM_DDR_INIT_SUCCESS;
}

#ifdef CONFIG_DEBUG_DDR
static void ddr_dump_regs(int ctl_id)
{
	printf("Register dump for CMCTR, PMCTR:\n");
	printf("%s : 0x%x\n", "DDR_INIT_END", PMCTR->DDR_INIT_END);
	printf("%s : 0x%x\n", "DDR_PIN_RET", PMCTR->DDR_PIN_RET);
	printf("%s : 0x%x\n", "SEL_CPLL", CMCTR->SEL_CPLL);
	printf("%s : 0x%x\n", "DIV_DDR0_CTR", CMCTR->DIV_DDR0_CTR);
	printf("%s : 0x%x\n", "DIV_DDR1_CTR", CMCTR->DIV_DDR1_CTR);

	printf("Register dump for Contorller %d:\n", ctl_id);
	printf("%s : 0x%x\n", "MSTR", DDRMC[ctl_id]->MSTR);
	printf("%s : 0x%x\n", "STAT", DDRMC[ctl_id]->STAT);
	printf("%s : 0x%x\n", "RFSHTMG", DDRMC[ctl_id]->RFSHTMG);
	printf("%s : 0x%x\n", "DRAMTMG0", DDRMC[ctl_id]->DRAMTMG0);
	printf("%s : 0x%x\n", "DRAMTMG1", DDRMC[ctl_id]->DRAMTMG1);
	printf("%s : 0x%x\n", "DRAMTMG2", DDRMC[ctl_id]->DRAMTMG2);
	printf("%s : 0x%x\n", "DRAMTMG3", DDRMC[ctl_id]->DRAMTMG3);
	printf("%s : 0x%x\n", "DRAMTMG4", DDRMC[ctl_id]->DRAMTMG4);
	printf("%s : 0x%x\n", "DRAMTMG5", DDRMC[ctl_id]->DRAMTMG5);
	printf("%s : 0x%x\n", "DRAMTMG6", DDRMC[ctl_id]->DRAMTMG6);
	printf("%s : 0x%x\n", "ZQCTL0", DDRMC[ctl_id]->ZQCTL0);
	printf("%s : 0x%x\n", "DFITMG0", DDRMC[ctl_id]->DFITMG0);
	printf("%s : 0x%x\n", "DFITMG1", DDRMC[ctl_id]->DFITMG1);
	printf("%s : 0x%x\n", "DFIUPD1", DDRMC[ctl_id]->DFIUPD1);
	printf("%s : 0x%x\n", "ADDRMAP0", DDRMC[ctl_id]->ADDRMAP0);
	printf("%s : 0x%x\n", "ADDRMAP1", DDRMC[ctl_id]->ADDRMAP1);
	printf("%s : 0x%x\n", "ADDRMAP2", DDRMC[ctl_id]->ADDRMAP2);
	printf("%s : 0x%x\n", "ADDRMAP3", DDRMC[ctl_id]->ADDRMAP3);
	printf("%s : 0x%x\n", "ADDRMAP4", DDRMC[ctl_id]->ADDRMAP4);
	printf("%s : 0x%x\n", "ADDRMAP5", DDRMC[ctl_id]->ADDRMAP5);
	printf("%s : 0x%x\n", "ADDRMAP6", DDRMC[ctl_id]->ADDRMAP6);
	printf("%s : 0x%x\n", "PCTRL0", DDRMC[ctl_id]->PCTRL0);
	printf("%s : 0x%x\n", "PCTRL1", DDRMC[ctl_id]->PCTRL1);
	printf("%s : 0x%x\n", "PCTRL2", DDRMC[ctl_id]->PCTRL2);

	printf("Register dump for PHY %d:\n", ctl_id);
	printf("%s : 0x%x\n", "PIR", DDRPHY[ctl_id]->PIR);
	printf("%s : 0x%x\n", "PGCR", DDRPHY[ctl_id]->PGCR);
	printf("%s : 0x%x\n", "PGSR", DDRPHY[ctl_id]->PGSR);
	printf("%s : 0x%x\n", "ACDLLCR", DDRPHY[ctl_id]->ACDLLCR);
	printf("%s : 0x%x\n", "PTR0", DDRPHY[ctl_id]->PTR0);
	printf("%s : 0x%x\n", "PTR1", DDRPHY[ctl_id]->PTR1);
	printf("%s : 0x%x\n", "PTR2", DDRPHY[ctl_id]->PTR2);
	printf("%s : 0x%x\n", "DXCCR", DDRPHY[ctl_id]->DXCCR);
	printf("%s : 0x%x\n", "DSGCR", DDRPHY[ctl_id]->DSGCR);
	printf("%s : 0x%x\n", "DCR", DDRPHY[ctl_id]->DCR);
	printf("%s : 0x%x\n", "DTPR0", DDRPHY[ctl_id]->DTPR0);
	printf("%s : 0x%x\n", "DTPR1", DDRPHY[ctl_id]->DTPR1);
	printf("%s : 0x%x\n", "DTPR2", DDRPHY[ctl_id]->DTPR2);
	printf("%s : 0x%x\n", "MR0", DDRPHY[ctl_id]->MR0);
	printf("%s : 0x%x\n", "MR1", DDRPHY[ctl_id]->MR1);
	printf("%s : 0x%x\n", "MR2", DDRPHY[ctl_id]->MR2);
	printf("%s : 0x%x\n", "ZQ0CR0", DDRPHY[ctl_id]->ZQ0CR0);
	printf("%s : 0x%x\n", "ZQ0CR1", DDRPHY[ctl_id]->ZQ0CR1);
	printf("%s : 0x%x\n", "ZQ0SR0", DDRPHY[ctl_id]->ZQ0SR0);
	printf("%s : 0x%x\n", "ZQ0SR1", DDRPHY[ctl_id]->ZQ0SR1);

	printf("%s : 0x%x\n", "DX0GCR", DDRPHY[ctl_id]->DX0GCR);
	printf("%s : 0x%x\n", "DX0GSR0", DDRPHY[ctl_id]->DX0GSR0);
	printf("%s : 0x%x\n", "DX0GSR1", DDRPHY[ctl_id]->DX0GSR1);
	printf("%s : 0x%x\n", "DX0DLLCR", DDRPHY[ctl_id]->DX0DLLCR);
	printf("%s : 0x%x\n", "DX0DQTR", DDRPHY[ctl_id]->DX0DQTR);
	printf("%s : 0x%x\n", "DX0DQSTR", DDRPHY[ctl_id]->DX0DQSTR);

	printf("%s : 0x%x\n", "DX1GCR", DDRPHY[ctl_id]->DX1GCR);
	printf("%s : 0x%x\n", "DX1GSR0", DDRPHY[ctl_id]->DX1GSR0);
	printf("%s : 0x%x\n", "DX1GSR1", DDRPHY[ctl_id]->DX1GSR1);
	printf("%s : 0x%x\n", "DX1DLLCR", DDRPHY[ctl_id]->DX1DLLCR);
	printf("%s : 0x%x\n", "DX1DQTR", DDRPHY[ctl_id]->DX1DQTR);
	printf("%s : 0x%x\n", "DX1DQSTR", DDRPHY[ctl_id]->DX1DQSTR);

	printf("%s : 0x%x\n", "DX2GCR", DDRPHY[ctl_id]->DX2GCR);
	printf("%s : 0x%x\n", "DX2GSR0", DDRPHY[ctl_id]->DX2GSR0);
	printf("%s : 0x%x\n", "DX2GSR1", DDRPHY[ctl_id]->DX2GSR1);
	printf("%s : 0x%x\n", "DX2DLLCR", DDRPHY[ctl_id]->DX2DLLCR);
	printf("%s : 0x%x\n", "DX2DQTR", DDRPHY[ctl_id]->DX2DQTR);
	printf("%s : 0x%x\n", "DX2DQSTR", DDRPHY[ctl_id]->DX2DQSTR);

	printf("%s : 0x%x\n", "DX3GCR", DDRPHY[ctl_id]->DX3GCR);
	printf("%s : 0x%x\n", "DX3GSR0", DDRPHY[ctl_id]->DX3GSR0);
	printf("%s : 0x%x\n", "DX3GSR1", DDRPHY[ctl_id]->DX3GSR1);
	printf("%s : 0x%x\n", "DX3DLLCR", DDRPHY[ctl_id]->DX3DLLCR);
	printf("%s : 0x%x\n", "DX3DQTR", DDRPHY[ctl_id]->DX3DQTR);
	printf("%s : 0x%x\n", "DX3DQSTR", DDRPHY[ctl_id]->DX3DQSTR);
}
#endif

u32 mcom_ddr_init(struct ddr_cfg *cfg0, struct ddr_cfg *cfg1,
		  struct ddr_freq *freq)
{
	struct ddr_cfg *cfg[2] = {cfg0, cfg1};
	u16 rc[2];
	int i;

	for (i = 0; i < 2; i++) {
		if (!cfg[i]) {
			printf("No configuration for DDRMC #%d\n", i);
			rc[i] = MCOM_DDR_NO_CFG;
			continue;
		}

		rc[i] = validate_cfg(cfg[i]);
		if (rc[i]) {
			printf("Invalid configuration for DDRMC #%d\n", i);
			continue;
		}

		rc[i] = ddr_init_phase1(cfg[i], freq);
	}

	if (rc[0] && rc[1])
		goto out;

	ddr_set_freq(freq);

	for (i = 0; i < 2; i++) {
		if (rc[i])
			continue;

		rc[i] = ddr_init_phase2(cfg[i], freq);

#ifdef CONFIG_DEBUG_DDR
		if ((rc[i] != MCOM_DDR_NO_CFG) && (rc[i] != MCOM_DDR_CFG_ERR))
			ddr_dump_regs(i);
#endif
	}

out:
	return rc[0] | (rc[1] << 16);
}
