/*
 * (C) Copyright 2015, ELVEES NeoTek CJSC
 *
 * Vasiliy Zasukhin <vzasukhin@elvees.com>
 * Anton Leontiev <aleontiev@elvees.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _BOOTROM_H
#define _BOOTROM_H

/*! \todo Doxygen documentation for all functions */
/*! \todo Rename functions */

void bootrom_set_cpu_stack(void);
unsigned bootrom_get_cpu_id(void);


/* UART types and functions */

typedef enum {
	BAUD9600   = 156,
	BAUD14400  = 104,
	BAUD19200  = 78,
	BAUD28800  = 52,
	BAUD38400  = 39,
	BAUD57600  = 26,
	BAUD115200 = 13,
	BAUD128000 = 11
} baudrate_t;

void bootrom_uart_config(baudrate_t baudrate);
void bootrom_uart_disable(void);
void bootrom_uart_putstr(char *str);
void bootrom_PrintUint32(unsigned int n);


/* SPI types and functions */

typedef struct {
	int errors;
	int mode_sync;
	unsigned int uart_file_accepted;
	unsigned int uart_total_bytes;
	unsigned int bset_spi0;
	unsigned int spiflash_offset_address;
	unsigned int spiflash_block_address;
} boot_config_t;

void bootrom_ConfigSPI0(void);
void bootrom_DisableSPI0(void);
void bootrom_ReadSPI0Flash(unsigned int address, unsigned char *buf,
		unsigned int size, boot_config_t *cfg);
void bootrom_WriteSPI0Flash(unsigned int address, unsigned char *buf,
		unsigned int size, boot_config_t *cfg);
void bootrom_SendOpcodeSPI0(unsigned char opcode);
void bootrom_ReadStatusRegisterSPI0Flash(void);


/* String functions */

int bootrom_StrLen(unsigned char *str);
unsigned char *bootrom_StrCpy(unsigned char *src, int size,
		unsigned char *dest);
int bootrom_StrCompare(const char *str1, unsigned char *str2, int startindex,
		int size);
int bootrom_StrIndexOf(unsigned char *str, unsigned char ch, int startindex,
		int endindex);
unsigned bootrom_StringToUint32(unsigned char *str, int startindex, int size);
int bootrom_MemoryCopy(unsigned int *src, unsigned int size, unsigned int *dest,
		unsigned int check_data);

int bootrom_set_data_package(unsigned int pAddress, unsigned int count,
		unsigned base_reg_address);
int bootrom_compress_and_dump_register_fields(
		const unsigned int *adr_mask_array,
		int array_size,
		unsigned int* out_array,
		unsigned int start_base_address_reg);


/* DDR types and functions */

#include "ddr_cfg.h"

void bootrom_umctl2_load(ddr3_t *mem_cfg, void *ddrmc_regs);
void bootrom_ddr3_pub_load(ddr3_t *mem_cfg, void *pub_regs);
void bootrom_init_start(void *pub_regs, unsigned int reset_type);
int bootrom_pub_init_cmpl_wait(void *pub_regs, unsigned int reset_type,
			       unsigned int check);
void bootrom_umctl2_norm_wait(void *ddrmc_regs);

/* SD/MMC types and functions */

struct SDMMC_REGS {
	volatile unsigned int SDMASA;
	volatile unsigned int BLKSIZECOUNT;
	volatile unsigned int ARGUMENT1;
	volatile unsigned int TRANSMODECOM;
	volatile unsigned int RESP1;
	volatile unsigned int RESP2;
	volatile unsigned int RESP3;
	volatile unsigned int RESP4;
	volatile unsigned char RESERVED0[4];
	volatile unsigned int PRSTATE;
	volatile unsigned int HOST_POW_CNTL;
	volatile unsigned int CLK_TIMEOUT_CTRL_SOFT_RST;
	volatile unsigned int INTSTS;
	volatile unsigned int INTSTSEN;
	volatile unsigned int INTSIGEN;
	volatile unsigned int HOST_CTRL_2;
	volatile unsigned char RESERVED1[192];
	volatile unsigned int EXT_REG_0;
	volatile unsigned int EXT_REG_1;
	volatile unsigned int EXT_REG_2;
	volatile unsigned int EXT_REG_3;
	volatile unsigned int EXT_REG_4;
	volatile unsigned int EXT_REG_5;
	volatile unsigned int EXT_REG_6;
	volatile unsigned int EXT_REG_7;
	volatile unsigned int EXT_REG_8;
	volatile unsigned int EXT_REG_9;
	volatile unsigned int EXT_REG_10;
	volatile unsigned char RESERVED2[20];
};

typedef struct {
    struct SDMMC_REGS *regs;
    unsigned int rca;	/* relative card address, filled after CMD3 */
    int isMMC;		/* 0 - SD, 1 - MMC, filled in function sd_acmd41
    			   after call cmd55 */
    int sdhc_mode;	/* 0 - SDSC, 1 - SDHC/SDXC, filled after ACMD41 */
    int version;	/* sd protocol version, 1 for version1.0 or 2 for
    			   version >=2.0, filled after CMD8 */
    int s18a;	/* support 1.8V on signal line. Filled after ACMD41 */
} sd_card_t;

int bootrom_sd_get_nom(void);
int bootrom_sd_struct_load(sd_card_t *sd);
int bootrom_sd_read_block(sd_card_t *sd, unsigned int block_num,
		unsigned int addr);


/* NAND types and functions */

typedef struct {
	union {
		unsigned int full_reg;
		struct {
			unsigned pkt_size:11;	/* packet size for read
						   operation */
			unsigned asyn_mode:3;	/* must always be 0 (other
						   timing mode is not
						   supported) */
			unsigned asyn_syn:1;	/* must always be 0 (only
						   asynchronous mode is
						   supported) */
			unsigned syn_mode:3;	/* must always be 0 (sync
						   interface is not yet
						   supported) */
			unsigned page_size:3;	/* flash device page size */
			unsigned num_addr_cycles:3;	/* number of address
							   cycles */
			unsigned dqs_buff_sel:4;/* must always be 6because other
						 timing mode is not supported */
			unsigned dma_buf_bound:3;/* dma buffer boundary */
		};
	} r0;
	union {
		unsigned int full_reg;
		struct {
			unsigned dma_en:2;	/* mdma enable bits (for read
						   operation should be 2) */
		};
	} r1;
} ctr_cfg_t;

typedef struct {
	union {
		unsigned int full_reg;
		struct {
			unsigned addr_col:13;
			unsigned addr_page:7;
			unsigned addr_block_1:7;
			unsigned addr_block_2:2;
		};
	} addr;				/* source address for read operation */

	unsigned int packet_cnt;	/* packet count */
	unsigned int sys_addr;		/* destinations address for read
					   operation*/
} tparams_t;

/* controller initialization */
void bootrom_nand_ctr_init(void *regs, const ctr_cfg_t *cfg);

/* reset command sending for specified chip select cs */
void bootrom_nand_reset(void *regs, unsigned int cs);

/* read from nand to specified system address.
   Function is blocking, return when read is finished.
   All interrupts are masked and reseted in function.*/
void bootrom_nand_read(void *regs, const tparams_t *params, unsigned int cs);

void bootrom_rtc_init(void);
void bootrom_ddr_ret(unsigned int ctl_num);
void bootrom_set_lpddr2_cfg(void *lpddr2_mem);
void bootrom_set_ddr3_cfg(void *ddr3_mem);

#endif /* _BOOTROM_H */
