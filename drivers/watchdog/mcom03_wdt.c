// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2022 RnD Center "ELVEES", JSC
 *
 */

#include <common.h>
#include <dm.h>
#include <dm/device_compat.h>
#include <wdt.h>
#include <mcom03_sip.h>

#define mcom03_wdt_sip(id) \
	mcom03_sip_smccc_smc(MCOM03_SIP_WDT, (id), 0, 0, 0, 0, 0, 0)
#define mcom03_wdt_sip_param(id, param) \
	mcom03_sip_smccc_smc(MCOM03_SIP_WDT, (id), (param), 0, 0, 0, 0, 0)

static int mcom03_wdt_reset(struct udevice *dev)
{
	if (mcom03_wdt_sip(MCOM03_SIP_WDT_IS_ENABLE))
		/* restart the watchdog counter */
		mcom03_wdt_sip(MCOM03_SIP_WDT_PING);

	return 0;
}

static int mcom03_wdt_stop(struct udevice *dev)
{
	mcom03_wdt_reset(dev);
	return 0;
}

static int mcom03_wdt_start(struct udevice *dev, u64 timeout, ulong flags)
{
	/* enable watchdog and set timeout in seconds */
	mcom03_wdt_sip_param(MCOM03_SIP_WDT_START, timeout / 1000UL);

	/* reset the watchdog */
	return mcom03_wdt_reset(dev);
}

static int mcom03_wdt_probe(struct udevice *dev)
{
	return mcom03_wdt_reset(dev);
}

static const struct wdt_ops mcom03_wdt_ops = {
	.start = mcom03_wdt_start,
	.reset = mcom03_wdt_reset,
	.stop = mcom03_wdt_stop,
};

static const struct udevice_id mcom03_wdt_ids[] = {
	{ .compatible = "elvees,mcom03-wdt" },
	{}
};

U_BOOT_DRIVER(mcom03_wdt) = {
	.name = "mcom03-wdt",
	.id = UCLASS_WDT,
	.of_match = mcom03_wdt_ids,
	.probe = mcom03_wdt_probe,
	.ops = &mcom03_wdt_ops,
	.flags = DM_FLAG_PRE_RELOC,
};
