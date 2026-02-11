/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2021-2026 RnD Center "ELVEES", JSC
 *
 * Configuration settings for the MCom-03 Bring-Up board
 */

#ifndef __MCOM03_H
#define __MCOM03_H

#include "mcom03-common.h"

#if !IS_ENABLED(CONFIG_FIT)
#define BOOT_TARGET_DEVICES(func) \
	BOOT_TARGET_DEVICES_MMC(func) \
	BOOT_TARGET_DEVICES_USB(func) \
	BOOT_TARGET_DEVICES_PXE(func) \
	BOOT_TARGET_DEVICES_DHCP(func)

#include <config_distro_bootcmd.h>
#endif

#define CFG_EXTRA_ENV_SETTINGS \
	MCOM03_COMMON_ENV_SETTINGS \
	BOOTENV

#endif
