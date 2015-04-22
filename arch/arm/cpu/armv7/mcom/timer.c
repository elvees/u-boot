/*
 * (C) Copyright 2007-2011
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/* Supports: Synopsys dw_timers
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/regs.h>

DECLARE_GLOBAL_DATA_PTR;

#define TIMER_INT_MASK	(0x1 << 2)	/* mask interrupt */
#define TIMER_EN		(0x1 << 0)	/* enable timer */

#define TIMER_CLOCK			CONFIG_TIMER_CLK_FREQ
#define COUNT_TO_USEC(x)	((x) / (TIMER_CLOCK / 1000000))
#define USEC_TO_COUNT(x)	((x) * (TIMER_CLOCK / 1000000))
#define TICKS_PER_HZ		(TIMER_CLOCK / CONFIG_SYS_HZ)
#define TICKS_TO_HZ(x)		((x) / TICKS_PER_HZ)

#define TIMER_LOAD_VAL		0xffffffff

/* read the 32-bit timer */
static ulong read_timer(void)
{
	dwc_timer_regs_t *timer0 = (dwc_timer_regs_t*)TIMERS_BASE;
	/*
	 * The hardware timer counts down, therefore we invert to
	 * produce an incrementing timer.
	 */
	return ~readl(&timer0->CURRENT_VALUE);
}

/* init timer register */
int timer_init(void)
{
	dwc_timer_regs_t *timer0 = (dwc_timer_regs_t*)TIMERS_BASE;
	timer0->CONTROL_REG = 0x5;
	writel(TIMER_INT_MASK | TIMER_EN, &timer0->CONTROL_REG);
	return 0;
}

/* timer without interrupts */
ulong get_timer(ulong base)
{
	return get_timer_masked() - base;
}

ulong get_timer_masked(void)
{
	/* current tick value */
	ulong now = TICKS_TO_HZ(read_timer());

	if (now >= gd->arch.lastinc)	/* normal (non rollover) */
		gd->arch.tbl += (now - gd->arch.lastinc);
	else {
		/* rollover */
		gd->arch.tbl += (TICKS_TO_HZ(TIMER_LOAD_VAL)
				- gd->arch.lastinc) + now;
	}
	gd->arch.lastinc = now;

	return gd->arch.tbl;
}

/* delay x useconds */
void __udelay(unsigned long usec)
{
	long tmo = USEC_TO_COUNT(usec);
	ulong now, last = read_timer();

	while (tmo > 0) {
		now = read_timer();
		if (now >= last)	/* normal (non rollover) */
			tmo -= now - last;
		else		/* rollover */
			tmo -= TIMER_LOAD_VAL - last + now;
		last = now;
	}
}

unsigned long long get_ticks(void)
{
	return get_timer(0);
}

ulong get_tbclk(void)
{
	return CONFIG_SYS_HZ;
}
