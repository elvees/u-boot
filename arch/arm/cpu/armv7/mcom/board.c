/*
 * (C) Copyright 2015
 * ELVEES NeoTek CJSC, <www.elvees-nt.com>
 *
 * Vasiliy Zasukhin <vzasukhin@elvees.com>
 *
 * Some init for MCom platform.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#ifdef CONFIG_SPL_BUILD
#include <spl.h>
#include <asm/arch/bootrom.h>
#endif
#include <asm/arch/clock.h>
#include <asm/arch/regs.h>

#define MEM_ACCESS(ADDR) (*((volatile uint32_t*)(ADDR)))
#define BOOTROM_COLD_RESET_BRANCH 0x0000019c

int spl_board_load_image(void)
{
	return 0;
}

#ifdef CONFIG_SPL_BUILD

u32 spl_boot_device(void)
{
	return BOOT_DEVICE_MMC1;
}

u32 spl_boot_mode(void)
{
	return MMCSD_MODE_RAW;
}

void board_init_f(ulong dummy)
{
	sys_t sys;

	if (bootrom_get_cpu_id() != 0) {

		/* FIXME
		 *
		 * The code in this block is actualy a hack.
		 * We should power down all cores except core0. Instead of that
		 * core1 simply polls certain spram address. When magic value is
		 * read (value is written by linux kernel) core1 jumps to the
		 * kernel code. Jump address is got from another spram address.
		 *
		*/

		uint32_t magic = 0xdeadbeef;
		uint32_t addr_for_magic = 0x2000fff8;
		uint32_t addr_for_jump_addr = 0x2000fff4;
		void (*jump_to_kernel)(void);

		while(MEM_ACCESS(addr_for_magic) != magic) {}
		invalidate_dcache_all();
		jump_to_kernel = (void (*)(void))MEM_ACCESS(addr_for_jump_addr);
		jump_to_kernel();
	}

	INIT_SYS_REGS(sys);

	/* Set ALWAYS_MISC0 to start address of bootrom _cold_reset
	 * branch. The SoC can be rebooted correctly after this.
	 */
	sys.PMCTR->ALWAYS_MISC0 = BOOTROM_COLD_RESET_BRANCH;

	/*
	 * Set BOOT_REMAP and ACP_CTL to 0x0 due to bug in L0-commutator
	 * (see bug #971).
	*/
	sys.SMCTR->BOOT_REMAP = 0x0;
	sys.SMCTR->ACP_CTL = 0x0;

	/* After changing remap we need instruction barrier for use actual
	 * bootrom API functions */
	asm volatile ("isb" ::: "memory");

	sys.CMCTR->DIV_MPU_CTR = 1;
	sys.CMCTR->DIV_ATB_CTR = 3;
	sys.CMCTR->DIV_APB_CTR = 1;
	sys.CMCTR->SEL_APLL = APLL_VALUE;
	if (APLL_VALUE != 0) {
		while ((sys.CMCTR->SEL_APLL & 0x80000000) == 0) {
		}
	}

	sys.CMCTR->SEL_CPLL = CPLL_VALUE;  // Need to setup before call dram_init()
	if (CPLL_VALUE) {
		while ((sys.CMCTR->SEL_CPLL & 0x80000000) == 0) {
		}
	}

	sys.CMCTR->DIV_SYS0_CTR = DIV_SYS0_CTR_VALUE;
	sys.CMCTR->DIV_SYS1_CTR = DIV_SYS1_CTR_VALUE;
	sys.CMCTR->SEL_SPLL = SPLL_VALUE;
	if (SPLL_VALUE != 0) {
		while ((sys.CMCTR->SEL_SPLL & 0x80000000) == 0) {
		}
	}

	preloader_console_init();

	dram_init();

	sys.CMCTR->GATE_SYS_CTR |= 0xc;  /* Enable clock for SDMMC0, SDMMC1 */
	sys.SDMMC0->EXT_REG_1 = (sys.SDMMC0->EXT_REG_1 & 0x00FFFFFF) |
			((SPLL_FREQ / 1000000) << 24);
	sys.SDMMC0->EXT_REG_2 &= ~0x38000000;  /* disable SDR50, SDR104, DDR50 */
	sys.SDMMC0->EXT_REG_6 &= ~0x39000000;  /* disable 1.8V mode */

	sys.SDMMC1->EXT_REG_1 = (sys.SDMMC1->EXT_REG_1 & 0x00FFFFFF) |
			((SPLL_FREQ / 1000000) << 24);
	sys.SDMMC1->EXT_REG_2 &= ~0x38000000;  /* disable SDR50, SDR104, DDR50 */
	sys.SDMMC1->EXT_REG_6 &= ~0x39000000;  /* disable 1.8V mode */

	sys.SDMMC1->EXT_REG_1 &= ~0x00080000;  /* embedded card */

	/*
	 * HACK: Write operations fails with some SD cards in HighSpeed mode.
	 * Disable HighSpeed mode for workaround as a dirty hack.
	 */
	sys.SDMMC0->EXT_REG_2 &= ~0x04000000;
	sys.SDMMC1->EXT_REG_2 &= ~0x04000000;

	/* Workaround for bug on some boards: SDMMC1 Card Detect pin
	 * is left floating */
	sys.SDMMC1->HOST_POW_CNTL |= 0xc0;

	/* Pull-up all pins for SDMMC1. For SDMMC0 pins already pulled-up in
	 * bootrom/spi loader */
	sys.SDMMC1->EXT_REG_7 = 0x0003ffff;
}
#endif

void reset_cpu(ulong addr)
{
}

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
	/* Enable D-cache. I-cache is already enabled in start.S */
	dcache_enable();
}
#endif

#ifdef CONFIG_DISPLAY_CPUINFO
int print_cpuinfo(void)
{
	puts("CPU:   MCom-compatible\n");
	return 0;
}
#endif
