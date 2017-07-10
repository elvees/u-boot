/*
 * Copyright 2015-2016 ELVEES NeoTek JSC
 * Copyright 2017 RnD Center "ELVEES", JSC
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef _REG_FIELDS_DDR_H
#define _REG_FIELDS_DDR_H

#include <linux/bitops.h>

/* DDRMC register field description */
#define MSTR_DDR3                 BIT(0)
#define MSTR_LPDDR2               BIT(2)
#define MSTR_BURST_RDWR           GENMASK(19, 16)
#define MSTR_ACTIVE_RANKS         GENMASK(27, 24)

#define STAT_OPER_MODE            GENMASK(2, 0)

#define RFSHTMG_TRFC_MIN          GENMASK(8, 0)
#define RFSHTMG_TRFC_NOM          GENMASK(27, 16)

#define DRAMTMG0_TRAS_MIN         GENMASK(5, 0)
#define DRAMTMG0_TRAS_MAX         GENMASK(14, 8)
#define DRAMTMG0_TFAW             GENMASK(21, 16)
#define DRAMTMG0_WR2PRE           GENMASK(29, 24)

#define DRAMTMG1_TRC              GENMASK(5, 0)
#define DRAMTMG1_RD2PRE           GENMASK(12, 8)
#define DRAMTMG1_TXP              GENMASK(20, 16)

#define DRAMTMG2_WR2RD            GENMASK(5, 0)
#define DRAMTMG2_RD2WR            GENMASK(12, 8)
#define DRAMTMG2_READ_LAT         GENMASK(21, 16)
#define DRAMTMG2_WRITE_LAT        GENMASK(29, 24)

#define DRAMTMG3_TMOD             GENMASK(9, 0)
#define DRAMTMG3_TMRD             GENMASK(17, 12)
#define DRAMTMG3_TMRW             GENMASK(29, 20)

#define DRAMTMG4_TRP              GENMASK(4, 0)
#define DRAMTMG4_TRRD             GENMASK(11, 8)
#define DRAMTMG4_TCCD             GENMASK(18, 16)
#define DRAMTMG4_TRCD             GENMASK(28, 24)

#define DRAMTMG5_TCKE             GENMASK(3, 0)
#define DRAMTMG5_TCKESR           GENMASK(13, 8)
#define DRAMTMG5_TCKSRE           GENMASK(19, 16)
#define DRAMTMG5_TCKSRX           GENMASK(27, 24)

#define DRAMTMG6_TCKCSX           GENMASK(3, 0)
#define DRAMTMG6_TCKDPDX          GENMASK(19, 16)
#define DRAMTMG6_TCKDPDE          GENMASK(27, 24)

#define ZQCTL0_TZQSHORT           GENMASK(9, 0)
#define ZQCTL0_TZQLONG            GENMASK(25, 16)

#define ZQCTL1_TZQSHORT_INTERVAL  GENMASK(19, 0)
#define ZQCTL1_TZQRESET           GENMASK(29, 20)

#define DFIMISC_INIT_COMPLETE_EN  BIT(0)

#define DFITMG0_TPHY_WRLAT        GENMASK(5, 0)
#define DFITMG0_TPHY_WRDATA       GENMASK(13, 8)
#define DFITMG0_RDDATA_EN         GENMASK(21, 16)
#define DFITMG0_CTRL_DELAY        GENMASK(28, 24)

#define DFIUPD1_UPD_INTER_MAX     GENMASK(7, 0)
#define DFIUPD1_UPD_INTER_MIN     GENMASK(23, 16)

#define INIT0_SKIP_DRAM_INIT      BIT(31)

#define ADDRMAP0_CS_BIT0          GENMASK(4, 0)
#define ADDRMAP0_CS_BIT1          GENMASK(12, 8)

#define ADDRMAP1_BANK_BIT0        GENMASK(3, 0)
#define ADDRMAP1_BANK_BIT1        GENMASK(11, 8)
#define ADDRMAP1_BANK_BIT2        GENMASK(19, 16)

#define ADDRMAP2_COL_BIT2         GENMASK(3, 0)
#define ADDRMAP2_COL_BIT3         GENMASK(11, 8)
#define ADDRMAP2_COL_BIT4         GENMASK(19, 16)
#define ADDRMAP2_COL_BIT5         GENMASK(27, 24)

#define ADDRMAP3_COL_BIT6         GENMASK(3, 0)
#define ADDRMAP3_COL_BIT7         GENMASK(11, 8)
#define ADDRMAP3_COL_BIT8         GENMASK(19, 16)
#define ADDRMAP3_COL_BIT9         GENMASK(27, 24)

#define ADDRMAP4_COL_BIT10        GENMASK(3, 0)
#define ADDRMAP4_COL_BIT11        GENMASK(11, 8)

#define ADDRMAP5_ROW_BIT0         GENMASK(3, 0)
#define ADDRMAP5_ROW_BIT1         GENMASK(11, 8)
#define ADDRMAP5_ROW_BIT2_10      GENMASK(19, 16)
#define ADDRMAP5_ROW_BIT11        GENMASK(27, 24)

#define ADDRMAP6_ROW_BIT12        GENMASK(3, 0)
#define ADDRMAP6_ROW_BIT13        GENMASK(11, 8)
#define ADDRMAP6_ROW_BIT14        GENMASK(19, 16)
#define ADDRMAP6_ROW_BIT15        GENMASK(27, 24)

#define ADDRMAP7_ROW_BIT16        GENMASK(3, 0)
#define ADDRMAP7_ROW_BIT17        GENMASK(11, 8)

#define PCTRL_PORT_EN             BIT(0)

/* DDR PHY register field description */
#define PIR_INIT                  BIT(0)
#define PIR_DLLSRST               BIT(1)
#define PIR_DLLLOCK               BIT(2)
#define PIR_ZCAL                  BIT(3)
#define PIR_ITMSRST               BIT(4)
#define PIR_DRAMRST               BIT(5)
#define PIR_DRAMINIT              BIT(6)
#define PIR_QSTRN                 BIT(7)
#define PIR_RVTRN                 BIT(8)

#define PGCR_DQSCFG               BIT(1)
#define PGCR_DFTCMP               BIT(2)
#define PGCR_RANKEN               GENMASK(21, 18)
#define PGCR_RFSHDT               GENMASK(28, 25)

#define PGSR_IDONE                BIT(0)
#define PGSR_DLDONE               BIT(1)
#define PGSR_ZCDONE               BIT(2)
#define PGSR_DIDONE               BIT(3)
#define PGSR_DTDONE               BIT(4)
#define PGSR_DTERR                BIT(5)
#define PGSR_DTIERR               BIT(6)
#define PGSR_RVERR                BIT(8)
#define PGSR_RVIERR               BIT(9)

#define ACDLLCR_DLLSRST           BIT(30)

#define PTR0_TDLLSRST             GENMASK(5, 0)
#define PTR0_TDLLLOCK             GENMASK(17, 6)
#define PTR0_TITMSRST             GENMASK(21, 18)

#define PTR1_TDINIT0              GENMASK(18, 0)
#define PTR1_TDINIT1              GENMASK(26, 19)

#define PTR2_TDINIT2              GENMASK(16, 0)
#define PTR2_TDINIT3              GENMASK(26, 17)

#define DXCCR_DXODT               BIT(0)
#define DXCCR_DQSRES              GENMASK(7, 4)
#define DXCCR_DQSNRES             GENMASK(11, 8)
#define DXCCR_AWDT                BIT(16)

#define DSGCR_DQSGX               GENMASK(7, 5)
#define DSGCR_DQSGE               GENMASK(10, 8)
#define DSGCR_NL2PD               BIT(24)
#define DSGCR_NL2OE               BIT(25)

#define DCR_DDRMD                 GENMASK(2, 0)
#define DCR_DDR8BANK              BIT(3)
#define DCR_DDRTYPE               GENMASK(9, 8)

#define DTPR0_TMRD                GENMASK(1, 0)
#define DTPR0_TRTP                GENMASK(4, 2)
#define DTPR0_TWTR                GENMASK(7, 5)
#define DTPR0_TRP                 GENMASK(11, 8)
#define DTPR0_TRCD                GENMASK(15, 12)
#define DTPR0_TRAS                GENMASK(20, 16)
#define DTPR0_TRRD                GENMASK(24, 21)
#define DTPR0_TRC                 GENMASK(30, 25)
#define DTPR0_TCCD                BIT(31)

#define DTPR1_TFAW                GENMASK(8, 3)
#define DTPR1_TMOD                GENMASK(10, 9)
#define DTPR1_TRFC                GENMASK(23, 16)
#define DTPR1_TDQSMIN             GENMASK(26, 24)
#define DTPR1_TDQSMAX             GENMASK(29, 27)

#define DTPR2_TXS                 GENMASK(9, 0)
#define DTPR2_TXP                 GENMASK(14, 10)
#define DTPR2_TCKE                GENMASK(18, 15)
#define DTPR2_TDLLK               GENMASK(28, 19)

#define MR0_DDR3_BL               GENMASK(1, 0)
#define MR0_DDR3_CL               GENMASK(6, 4)
#define MR0_DDR3_WR               GENMASK(11, 9)

#define MR1_DDR3_DE               BIT(0)
#define MR1_DDR3_DIC_0            BIT(1)
#define MR1_DDR3_DIC_1            BIT(5)
#define MR1_DDR3_RTT_0            BIT(2)
#define MR1_DDR3_RTT_1            BIT(6)
#define MR1_DDR3_RTT_2            BIT(9)
#define MR1_DDR3_AL               GENMASK(4, 3)

#define MR1_LPDDR2_BL             GENMASK(2, 0)
#define MR1_LPDDR2_BT             BIT(3)
#define MR1_LPDDR2_WC             BIT(4)
#define MR1_LPDDR2_NWR            GENMASK(7, 5)

#define MR2_DDR3_CWL              GENMASK(5, 3)
#define MR2_DDR3_RTTWR            GENMASK(10, 9)

#define MR2_LPDDR2_RL_WL          GENMASK(3, 0)

#define MR3_LPDDR2_DS             GENMASK(3, 0)

#define ZQ0CR1_ZPROG              GENMASK(7, 0)

#endif /* _REG_FIELDS_DDR_H */
