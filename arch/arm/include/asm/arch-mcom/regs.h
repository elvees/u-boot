/*
 * Copyright 2013-2016 ELVEES NeoTek JSC
 * Copyright 2017 RnD Center "ELVEES", JSC
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef REGS_H
#define REGS_H

#include <compiler.h>

#define DDRMC0_BASE 0x37204000
#define DDRPHY0_BASE 0x37205000
#define DDRMC1_BASE 0x37206000
#define DDRPHY1_BASE 0x37207000
#define NFCM_BASE   0x38007000
#define SDMMC0_BASE 0x3800B000
#define SDMMC1_BASE 0x3800D000
#define TIMERS_BASE 0x38026000
#define RTC_BASE    0x38027000
#define UART0_BASE  0x38028000
#define BIST_BRIDGE_BASE 0x38035000
#define MFBSP0_BASE 0x38086000
#define MFBSP1_BASE 0x38088000
#define CMCTR_BASE  0x38094000
#define PMCTR_BASE  0x38095000
#define SMCTR_BASE  0x38096000

#define INIT_SYS_REGS(r) {\
r.DDRMC0=(ddrmc_t*)DDRMC0_BASE;\
r.DDRPHY0=(ddrphy_t*)DDRPHY0_BASE;\
r.DDRMC1=(ddrmc_t*)DDRMC1_BASE;\
r.DDRPHY1=(ddrphy_t*)DDRPHY1_BASE;\
r.NFCM=(nfcm_regs_t*)NFCM_BASE;\
r.SDMMC0=(sdmmc_regs_t*)SDMMC0_BASE;\
r.SDMMC1=(sdmmc_regs_t*)SDMMC1_BASE;\
r.TIMER0=(dwc_timer_regs_t*)TIMERS_BASE;\
r.RTC=(rtc_regs_t*)RTC_BASE;\
r.UART0=(uart_regs_t*)UART0_BASE;\
r.BIST_BRIDGE=(bist_bridge_regs_t*)BIST_BRIDGE_BASE;\
r.MFBSP0=(mfbsp_t*)MFBSP0_BASE;\
r.MFBSP1=(mfbsp_t*)MFBSP1_BASE;\
r.CMCTR=(cmctr_t*)CMCTR_BASE;\
r.PMCTR=(pmctr_t*)PMCTR_BASE;\
r.SMCTR=(smctr_t*)SMCTR_BASE;}

typedef struct {
	volatile uint32_t SDMASA;
	volatile uint32_t BLKSIZECOUNT;
	volatile uint32_t ARGUMENT1;
	volatile uint32_t TRANSMODECOM;
	volatile uint32_t RESP1;
	volatile uint32_t RESP2;
	volatile uint32_t RESP3;
	volatile uint32_t RESP4;
	volatile uint32_t RESERVED0;
	volatile uint32_t PRSTATE;
	volatile uint32_t HOST_POW_CNTL;
	volatile uint32_t CLK_TIMEOUT_CTRL_SOFT_RST;
	volatile uint32_t INTSTS;
	volatile uint32_t INTSTSEN;
	volatile uint32_t INTSIGEN;
	volatile uint32_t HOST_CTRL_2;
	volatile uint32_t RESERVED1[48];
	volatile uint32_t EXT_REG_0;
	volatile uint32_t EXT_REG_1;
	volatile uint32_t EXT_REG_2;
	volatile uint32_t EXT_REG_3;
	volatile uint32_t EXT_REG_4;
	volatile uint32_t EXT_REG_5;
	volatile uint32_t EXT_REG_6;
	volatile uint32_t EXT_REG_7;
	volatile uint32_t EXT_REG_8;
	volatile uint32_t EXT_REG_9;
	volatile uint32_t EXT_REG_10;
	volatile uint32_t RESERVED2[5];
} sdmmc_regs_t;

typedef struct {
	volatile uint32_t LOAD_COUNT;
	volatile uint32_t CURRENT_VALUE;
	volatile uint32_t CONTROL_REG;
	volatile uint32_t EOI;
	volatile uint32_t INT_STATUS;
} dwc_timer_regs_t;

typedef struct {
	volatile uint32_t PACKET;
	volatile uint32_t MEM_ADDR;
	volatile uint32_t MEM_ADDR2;
	volatile uint32_t COMMAND;
	volatile uint32_t PROGRAM;
	volatile uint32_t INTSTSEN;
	volatile uint32_t INTSIGEN;
	volatile uint32_t INTSTS;
	volatile uint32_t ID1;
	volatile uint32_t RESERVED0;
	volatile uint32_t FLASHSTS;
	volatile uint32_t TIMING;
	volatile uint32_t BUF_DATA_PORT_REG;
	volatile uint32_t ECC;
	volatile uint32_t ECC_ERROR_COUNT;
	volatile uint32_t ECC_SPARE_COMMAND;
	volatile uint32_t ERROR_COUNT_1BIT;
	volatile uint32_t ERROR_COUNT_2BIT;
	volatile uint32_t ERROR_COUNT_3BIT;
	volatile uint32_t ERROR_COUNT_4BIT;
	volatile uint32_t DMAADDR;
	volatile uint32_t DMABUFBOUND;
	volatile uint32_t RESERVED1;
	volatile uint32_t ERROR_COUNT_5BIT;
	volatile uint32_t ERROR_COUNT_6BIT;
	volatile uint32_t ERROR_COUNT_7BIT;
	volatile uint32_t ERROR_COUNT_8BIT;
} nfcm_regs_t;

typedef struct {
	volatile uint32_t ID;
	volatile uint32_t CTRL;
	volatile uint32_t TIME;
	volatile uint32_t DATE;
	volatile uint32_t TALRM;
	volatile uint32_t DALRM;
	volatile uint32_t STAT;
	volatile uint32_t TCNT;
	volatile uint32_t TCUR;
} rtc_regs_t;

typedef struct {
	volatile uint32_t DLL;
	volatile uint32_t DLH;
	volatile uint32_t FCR;
	volatile uint32_t LCR;
	volatile uint32_t MCR;
	volatile uint32_t LSR;
	volatile uint32_t MSR;
	volatile uint32_t SCR;
	volatile uint32_t LPDLL;
	volatile uint32_t LPDLH;
	volatile uint32_t RESERVED0[2];
	volatile uint32_t SRBR[16];
	volatile uint32_t RESERVED1[3];
	volatile uint32_t USR;
	volatile uint32_t TFL;
	volatile uint32_t RFL;
	volatile uint32_t SRR;
	volatile uint32_t SRTS;
	volatile uint32_t SBCR;
	volatile uint32_t RESERVED2;
	volatile uint32_t SFE;
	volatile uint32_t SRT;
	volatile uint32_t STET;
	volatile uint32_t HTX;
} uart_regs_t;

typedef struct {
	volatile uint32_t ID;
	volatile uint32_t STAT;
	volatile uint32_t SEQU;
	volatile uint32_t MODE;
	volatile uint32_t POFF;
	volatile uint32_t UDATA[64];
} bist_bridge_regs_t;

typedef struct {
	union {
		volatile uint32_t TX;
		volatile uint32_t RX;
	};
	volatile uint32_t CSR;
	volatile uint32_t DIR;
	volatile uint32_t GPIO_DR;
	volatile uint32_t TCTR;
	volatile uint32_t RCTR;
	volatile uint32_t TSR;
	volatile uint32_t RSR;
	volatile uint32_t TCTR_RATE;
	volatile uint32_t RCRE_RATE;
	volatile uint32_t TSTART;
	volatile uint32_t RSTART;
	volatile uint32_t EMERG;
	volatile uint32_t IMASK;
} mfbsp_t;

typedef struct {
	// CMCTR_MPU
	volatile uint32_t RESERVED0;
	volatile uint32_t DIV_MPU_CTR;
	volatile uint32_t DIV_ATB_CTR;
	volatile uint32_t DIV_APB_CTR;
	volatile uint32_t RESERVED1;
	volatile uint32_t GATE_MPU_CTR;
	volatile uint32_t RESERVED2[2];
	/* CMCTR_CORE */
	volatile uint32_t RESERVED3;
	volatile uint32_t RESERVED4;
	volatile uint32_t RESERVED5;
	volatile uint32_t DIV_GPU_CTR;
	volatile uint32_t DIV_DDR0_CTR;
	volatile uint32_t DIV_DDR1_CTR;
	volatile uint32_t DIV_NFC_CTR;
	volatile uint32_t DIV_NOR_CTR;
	volatile uint32_t DIV_SYS0_CTR;
	volatile uint32_t DIV_SYS1_CTR;
	volatile uint32_t GATE_CORE_CTR;
	volatile uint32_t GATE_SYS_CTR;
	volatile uint32_t RESERVED6[4];
	/* CMCTR_DSP */
	volatile uint32_t RESERVED7;
	volatile uint32_t RESERVED8;
	volatile uint32_t GATE_DSP_CTR;
	volatile uint32_t RESERVED9[5];
	/* CLKOUT */
	volatile uint32_t RESERVED10;
	volatile uint32_t DIV_CLKOUT;
	volatile uint32_t GATE_CLKOUT;
	volatile uint32_t RESERVED11;
	/* LS_ENABLE */
	volatile uint32_t LS_ENABLE;
	volatile uint32_t RESERVED12[27];
	/* PLL */
	volatile uint32_t SEL_APLL;
	volatile uint32_t SEL_CPLL;
	volatile uint32_t SEL_DPLL;
	volatile uint32_t SEL_SPLL;
	volatile uint32_t SEL_VPLL;
	volatile uint32_t SEL_UPLL;
} cmctr_t;

#define CMCTR_GATE_SYS_CTR_NFC_EN	BIT(21)
#define CMCTR_GATE_SYS_CTR_SPI1_EN	BIT(20)
#define CMCTR_GATE_SYS_CTR_SPI0_EN	BIT(19)
#define CMCTR_GATE_SYS_CTR_I2C0_EN	BIT(16)
#define CMCTR_GATE_SYS_CTR_USBIC_EN	BIT(5)
#define CMCTR_GATE_SYS_CTR_EMAC_EN	BIT(4)
#define CMCTR_GATE_SYS_CTR_SDMMC1_EN	BIT(3)
#define CMCTR_GATE_SYS_CTR_SDMMC0_EN	BIT(2)
#define CMCTR_GATE_SYS_CTR_SYS_EN	BIT(0)

typedef struct {
	volatile uint32_t RESERVED0;
	volatile uint32_t SYS_PWR_DOWN;
	volatile uint32_t RESERVED1;
	volatile uint32_t SYS_PWR_STATUS;
	volatile uint32_t SYS_PWR_IMASK;
	volatile uint32_t SYS_PWR_IRSTAT;
	volatile uint32_t SYS_PWR_ISTAT;
	volatile uint32_t SYS_PWR_ICLR;
	volatile uint32_t SYS_PWR_DELAY;
	volatile uint32_t DDR_PIN_RET;
	volatile uint32_t DDR_INIT_END;
	volatile uint32_t WARM_RST_EN;
	volatile uint32_t WKP_IMASK;
	volatile uint32_t WKP_IRSTAT;
	volatile uint32_t WKP_ISTAT;
	volatile uint32_t WKP_ICLR;
	volatile uint32_t SW_RST;
	volatile uint32_t WARM_RST_STATUS;
	volatile uint32_t PDM_RST_STATUS;
	volatile uint32_t VMODE;
	volatile uint8_t CPU0_WKP_MASK[16];
	volatile uint8_t CPU1_WKP_MASK[16];
	volatile uint32_t ALWAYS_MISC0;
	volatile uint32_t ALWAYS_MISC1;
	volatile uint32_t WARM_BOOT_OVRD;
	volatile uint32_t CORE_PWR_UP;
	volatile uint32_t CORE_PWR_DOWN;
	volatile uint32_t RESERVED2;
	volatile uint32_t CORE_PWR_STATUS;
	volatile uint32_t CORE_PWR_IMASK;
	volatile uint32_t CORE_PWR_IRSTAT;
	volatile uint32_t CORE_PWR_ISTAT;
	volatile uint32_t CORE_PWR_ICLR;
	volatile uint32_t CORE_PWR_DELAY;
} pmctr_t;

typedef struct {
	volatile uint32_t BOOT;
	volatile uint32_t BOOT_REMAP;
	volatile uint32_t MFU_CFGNMFI;
	volatile uint32_t DDR_REMAP;
	volatile uint32_t CPU_SECURE_CTR;
	volatile uint32_t ACP_CTL;
	volatile uint32_t RESERVED1[3];
	volatile uint32_t MIPI_MUX;
	volatile uint32_t CHIP_ID;
	volatile uint32_t CHIP_CONFIG;
	volatile uint32_t EMA_ARM;
	volatile uint32_t EMA_L2;
	volatile uint32_t EMA_DSP;
	volatile uint32_t EMA_CORE;
	volatile uint32_t IOPULL_CTR;
	volatile uint32_t COMM_DLOCK;
} smctr_t;

enum SMCTR_BOOT {
	SMCTR_BOOT_NOR_FLASH = 0,
	SMCTR_BOOT_NAND_FLASH,
	SMCTR_BOOT_UART0,
	SMCTR_BOOT_SPI0,
	SMCTR_BOOT_SDMMC0,
};

typedef struct {
	volatile uint32_t MSTR;
	volatile uint32_t STAT;
	volatile uint8_t RESERVED0[12];
	volatile uint32_t MRCTRL1;
	volatile uint8_t RESERVED1[12];
	volatile uint32_t DERATEINT;
	volatile uint8_t RESERVED2[8];
	volatile uint32_t PWRCTL;
	volatile uint8_t RESERVED3[28];
	volatile uint32_t RFSHCTL0;
	volatile uint8_t RESERVED4[12];
	volatile uint32_t RFSHCTL3;
	volatile uint32_t RFSHTMG;
	volatile uint8_t RESERVED5[104];
	volatile uint32_t INIT0;
	volatile uint32_t INIT1;
	volatile uint32_t INIT2;
	volatile uint32_t INIT3;
	volatile uint32_t INIT4;
	volatile uint32_t INIT5;
	volatile uint8_t RESERVED6[24];
	volatile uint32_t DRAMTMG0;
	volatile uint32_t DRAMTMG1;
	volatile uint32_t DRAMTMG2;
	volatile uint32_t DRAMTMG3;
	volatile uint32_t DRAMTMG4;
	volatile uint32_t DRAMTMG5;
	volatile uint32_t DRAMTMG6;
	volatile uint8_t RESERVED7[100];
	volatile uint32_t ZQCTL0;
	volatile uint32_t ZQCTL1;
	volatile uint8_t RESERVED8[8];
	volatile uint32_t DFITMG0;
	volatile uint32_t DFITMG1;
	volatile uint8_t RESERVED9[8];
	volatile uint32_t DFIUPD0;
	volatile uint32_t DFIUPD1;
	volatile uint8_t RESERVED10[8];
	volatile uint32_t DFIMISC;
	volatile uint8_t RESERVED11[76];
	volatile uint32_t ADDRMAP0;
	volatile uint32_t ADDRMAP1;
	volatile uint32_t ADDRMAP2;
	volatile uint32_t ADDRMAP3;
	volatile uint32_t ADDRMAP4;
	volatile uint32_t ADDRMAP5;
	volatile uint32_t ADDRMAP6;
	volatile uint8_t RESERVED12[628];
	volatile uint32_t PCTRL0;
	volatile uint8_t RESERVED13[172];
	volatile uint32_t PCTRL1;
	volatile uint8_t RESERVED14[172];
	volatile uint32_t PCTRL2;
	volatile uint8_t RESERVED15[12];
} ddrmc_t;

typedef struct {
	volatile uint32_t RIDR;
	volatile uint32_t PIR;
	volatile uint32_t PGCR;
	volatile uint32_t PGSR;
	volatile uint8_t RESERVED0[4];
	volatile uint32_t ACDLLCR;
	volatile uint32_t PTR0;
	volatile uint32_t PTR1;
	volatile uint32_t PTR2;
	volatile uint8_t RESERVED1[4];
	volatile uint32_t DXCCR;
	volatile uint32_t DSGCR;
	volatile uint32_t DCR;
	volatile uint32_t DTPR0;
	volatile uint32_t DTPR1;
	volatile uint32_t DTPR2;
	volatile uint32_t MR0;
	volatile uint32_t MR1;
	volatile uint32_t MR2;
	volatile uint32_t MR3;
	volatile uint8_t RESERVED2[112];
	volatile uint32_t DCUAR;
	volatile uint32_t DCUDR;
	volatile uint32_t DCURR;
	volatile uint32_t DCULR;
	volatile uint32_t DCUGCR;
	volatile uint32_t DCUTPR;
	volatile uint32_t DCUSR0;
	volatile uint8_t RESERVED4[68];
	volatile uint32_t BISTUDPR;
	volatile uint8_t RESERVED5[92];
	volatile uint32_t ZQ0CR0;
	volatile uint32_t ZQ0CR1;
	volatile uint32_t ZQ0SR0;
	volatile uint32_t ZQ0SR1;
	volatile uint8_t RESERVED7[48];
	volatile uint32_t DX0GCR;
	volatile uint32_t DX0GSR0;
	volatile uint32_t DX0GSR1;
	volatile uint32_t DX0DLLCR;
	volatile uint32_t DX0DQTR;
	volatile uint32_t DX0DQSTR;
	volatile uint8_t RESERVED9[40];
	volatile uint32_t DX1GCR;
	volatile uint32_t DX1GSR0;
	volatile uint32_t DX1GSR1;
	volatile uint32_t DX1DLLCR;
	volatile uint32_t DX1DQTR;
	volatile uint32_t DX1DQSTR;
	volatile uint8_t RESERVED10[40];
	volatile uint32_t DX2GCR;
	volatile uint32_t DX2GSR0;
	volatile uint32_t DX2GSR1;
	volatile uint32_t DX2DLLCR;
	volatile uint32_t DX2DQTR;
	volatile uint32_t DX2DQSTR;
	volatile uint8_t RESERVED11[40];
	volatile uint32_t DX3GCR;
	volatile uint32_t DX3GSR0;
	volatile uint32_t DX3GSR1;
	volatile uint32_t DX3DLLCR;
	volatile uint32_t DX3DQTR;
	volatile uint32_t DX3DQSTR;
	volatile uint8_t RESERVED12[360];
} ddrphy_t;

typedef struct {
	ddrmc_t *DDRMC0;
	ddrphy_t *DDRPHY0;
	ddrmc_t *DDRMC1;
	ddrphy_t *DDRPHY1;
	nfcm_regs_t *NFCM;
	sdmmc_regs_t *SDMMC0;
	sdmmc_regs_t *SDMMC1;
	dwc_timer_regs_t *TIMER0;
	rtc_regs_t *RTC;
	uart_regs_t *UART0;
	bist_bridge_regs_t *BIST_BRIDGE;
	mfbsp_t *MFBSP0;
	mfbsp_t *MFBSP1;
	cmctr_t *CMCTR;
	pmctr_t *PMCTR;
	smctr_t *SMCTR;
} sys_t;

#endif /* REGS_H */
