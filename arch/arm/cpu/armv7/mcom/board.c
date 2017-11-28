/*
 * Some init for MCom platform
 *
 * Copyright 2015 ELVEES NeoTek CJSC
 * Copyright 2015-2016 ELVEES NeoTek JSC
 * Copyright 2017 RnD Center "ELVEES", JSC
 *
 * Vasiliy Zasukhin <vzasukhin@elvees.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#ifdef CONFIG_SPL_BUILD
#include <spl.h>
#include <asm/arch/bootrom.h>
#include <asm/arch/ddr.h>
#endif
#include <asm/arch/clock.h>
#include <asm/arch/regs.h>
#include <watchdog.h>

#include <asm/io.h>

#define BOOTROM_COLD_RESET_BRANCH 0x0000019c

#define SMCTR_BASE 0x38096000
#define SMCTR_BOOT_REMAP (SMCTR_BASE + 0x4)

#define ARM_PERIPHBASE 0x39000000
#define SCU_BASE ARM_PERIPHBASE
#define SCU_POWER_STATUS (SCU_BASE + 0x8)
#define SCU_PM_NORMAL	0
#define SCU_PM_DORMANT	2
#define SCU_PM_POWEROFF	3

#define PMCTR_BASE 0x38095000
#define PMCTR_SYS_PWR_STATUS (PMCTR_BASE + 0x0c)

#ifdef CONFIG_SPL_BUILD
static void cpu_poweroff(const u32 cpu)
{
	u32 val;

	dcache_disable();
	invalidate_dcache_all();

	val = readl(SCU_POWER_STATUS);
	val |= SCU_PM_POWEROFF << (cpu * 8);
	writel(val, SCU_POWER_STATUS);

	wfi();
}

static void cpu_waitoff(const u32 cpu)
{
	while (!(readl(PMCTR_SYS_PWR_STATUS) & BIT(cpu + 1)))
		continue;
}
#endif

/*
 * According to U-Boot documentation, this function is called before the stack
 * is initialized and should be written in assembler. For MCom it uses Bootrom
 * stack and can be written in C.
 *
 * BUG: U-Boot can not be started without running Bootrom, for example directly
 * over JTAG.
 */
void lowlevel_init(void)
{
#ifdef CONFIG_SPL_BUILD
	u32 cpu = bootrom_get_cpu_id();

	if (cpu != 0)
		cpu_poweroff(cpu);
	else
		cpu_waitoff(1);
#endif
}

/*
 * TODO: Using structs to access registers is generally not a good idea.
 * Change this to use defines.
 */
#ifdef CONFIG_SPL_BUILD
void board_init_f(ulong dummy)
{
	sys_t sys;
	INIT_SYS_REGS(sys);

	/* DDR retention mode should be disabled as soon as possible
	 * to avoid large current on DDRx_VDDQ (see rf#1160).
	 */
	sys.PMCTR->DDR_PIN_RET = 0;
	bootrom_uart_putstr("DDR retention disabled\n");

	/* Set ALWAYS_MISC0 to start address of bootrom _cold_reset
	 * branch. The SoC can be rebooted correctly after this.
	 */
	sys.PMCTR->ALWAYS_MISC0 = BOOTROM_COLD_RESET_BRANCH;

#ifdef CONFIG_HW_WATCHDOG
	hw_watchdog_init();
	bootrom_uart_putstr("Watchdog enabled\n");
#endif

	/* Disable boot memory remapping due to the ACP bug (see rf#971) */
	sys.SMCTR->BOOT_REMAP = 0;

	/* Set default configuration for the ACP (see rf#972) */
	sys.SMCTR->ACP_CTL = 0;

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

	sys.CMCTR->DIV_SYS0_CTR = DIV_SYS0_CTR_VALUE;
	sys.CMCTR->DIV_SYS1_CTR = DIV_SYS1_CTR_VALUE;
	sys.CMCTR->SEL_SPLL = SPLL_VALUE;
	if (SPLL_VALUE != 0) {
		while ((sys.CMCTR->SEL_SPLL & 0x80000000) == 0) {
		}
	}

	preloader_console_init();

#ifdef CONFIG_TARGET_SALUTE_PM
	/* Enable I2C0 clock here since DDR power on Salute-EL24PM1
	 * is enabled using I2C0.*/
	sys.CMCTR->GATE_SYS_CTR |= CMCTR_GATE_SYS_CTR_I2C0_EN;

	spl_early_init();
#endif

	int rc = dram_init();
	/* It makes no sence to continue booting if DDRMC #0 initialization
	 * is failed */
	if (ddr_getrc(rc, 0))
		hang();

	/* Enable clock frequency for SDMMC0 and SDMMC1 */
	sys.CMCTR->GATE_SYS_CTR |= CMCTR_GATE_SYS_CTR_SDMMC0_EN;
	sys.CMCTR->GATE_SYS_CTR |= CMCTR_GATE_SYS_CTR_SDMMC1_EN;

	sys.SDMMC0->EXT_REG_1 = (sys.SDMMC0->EXT_REG_1 & 0x00FFFFFF) |
			((SPLL_FREQ / 1000000) << 24);
	sys.SDMMC0->EXT_REG_1 &= ~0x00010000;  /* disable SDMA */
	sys.SDMMC0->EXT_REG_1 &= ~0x00008000;  /* disable ADMA2 */
	sys.SDMMC0->EXT_REG_2 &= ~0x38000000;  /* disable SDR50, SDR104, DDR50 */
	sys.SDMMC0->EXT_REG_6 &= ~0x39000000;  /* disable 1.8V mode */

	sys.SDMMC1->EXT_REG_1 = (sys.SDMMC1->EXT_REG_1 & 0x00FFFFFF) |
			((SPLL_FREQ / 1000000) << 24);
	sys.SDMMC1->EXT_REG_1 &= ~0x00010000;  /* disable SDMA */
	sys.SDMMC1->EXT_REG_1 &= ~0x00008000;  /* disable ADMA2 */
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

	/* Pull-up all pins for SDMMC0 and SDMMC1 */
	sys.SDMMC0->EXT_REG_7 = 0x0003ffff;
	sys.SDMMC1->EXT_REG_7 = 0x0003ffff;

	/* Enable clock frequency for SPI0 */
	sys.CMCTR->GATE_SYS_CTR |= CMCTR_GATE_SYS_CTR_SPI0_EN;

	/* Enable clock frequency for GEMAC */
	sys.CMCTR->GATE_SYS_CTR |= CMCTR_GATE_SYS_CTR_EMAC_EN;
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
