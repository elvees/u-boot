// SPDX-License-Identifier:     GPL-2.0+
/*
 * Copyright 2020 RnD Center "ELVEES", JSC
 */

#include <common.h>
#include <dm.h>
#include <sysreset.h>
#include <asm/io.h>

#define PMCTR_WARM_RST_EN               0x2c
#define PMCTR_SW_RST                    0x40
#define PMCTR_WARM_RST_STATUS           0x44
#define PMCTR_WARM_RST_STATUS_WDT       BIT(4)
#define PMCTR_WARM_RST_STATUS_CPU0_WDT  BIT(2)
#define PMCTR_WARM_RST_STATUS_SW        BIT(1)
#define PMCTR_WARM_RST_STATUS_EXT       BIT(0)

struct mcom02_pdata {
	void *base;
};

static inline void pmctr_writel(struct mcom02_pdata *pdata, uint val, uint reg)
{
	writel(val, pdata->base + reg);
}

static inline u32 pmctr_readl(struct mcom02_pdata *pdata, uint reg)
{
	return readl(pdata->base + reg);
}

static int mcom02_sysreset_request(struct udevice *dev, enum sysreset_t type)
{
	switch (type) {
	case SYSRESET_COLD: {
		struct mcom02_pdata *pdata = dev_get_platdata(dev);

		/* Reset is too fast, does not have time to print the EOL */
		udelay(200);

		/* Reset the cpu by setting software reset bit */
		pmctr_writel(pdata, 0x1, PMCTR_WARM_RST_EN);
		pmctr_writel(pdata, 0x1, PMCTR_SW_RST);

		/* Loop forever */
		while (1)
			;

		return 0;
	}
	default:
		return -EPROTONOSUPPORT;
	}

	return -EINPROGRESS;
}

static int mcom02_sysreset_get_status(struct udevice *dev, char *buf, int size)
{
	struct mcom02_pdata *pdata = dev_get_platdata(dev);
	ulong rst = pmctr_readl(pdata, PMCTR_WARM_RST_STATUS);
	int res;

	res = strlcpy(buf, "Reset status: ", size);
	buf += res;
	size -= res;

	/* Due to rf#3929 we cannot determine the type of warm reset,
	 * thus we distinguish only power-on and warm reset.
	 */
	if (rst == 0)
		strlcpy(buf, "Power-on\n", size);
	else
		strlcpy(buf, "Software/Watchdog/External\n", size);

	return 0;
}

static int mcom02_ofdata_to_platdata(struct udevice *dev)
{
	struct mcom02_pdata *pdata = dev_get_platdata(dev);

	/* Get the controller base address */
	pdata->base = (void *)dev_read_addr_index(dev, 0);
	if ((fdt_addr_t)pdata->base == FDT_ADDR_T_NONE)
		return -EINVAL;

	return 0;
}

static const struct udevice_id mcom02_sysreset_ids[] = {
	{ .compatible = "elvees,mcom-pmctr" },
	{ /* sentinel */ }
};

static struct sysreset_ops mcom02_sysreset = {
	.request = mcom02_sysreset_request,
	.get_status = mcom02_sysreset_get_status,
};

U_BOOT_DRIVER(mcom02_sysreset) = {
	.name = "mcom02-sysreset",
	.id = UCLASS_SYSRESET,
	.of_match = mcom02_sysreset_ids,
	.ops = &mcom02_sysreset,
	.platdata_auto_alloc_size = sizeof(struct mcom02_pdata),
	.ofdata_to_platdata = mcom02_ofdata_to_platdata,
};
