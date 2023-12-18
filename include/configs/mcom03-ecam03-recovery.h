/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2023 RnD Center "ELVEES", JSC
 *
 * Recovery configuration settings for the ECAM03 boards
 */

#ifndef __MCOM03_ECAM03_RECOVERY_H
#define __MCOM03_ECAM03_RECOVERY_H

#include "mcom03-common.h"

#define BOOTENV_DEV_ECAM(devtypeu, devtypel, instance) \
	"boot_recovery=true\0" \
	"bootcmd_" #devtypel #instance "=" \
	"devtype=mmc;" \
	"devnum=" #instance ";" \
	"distro_bootpart=4;" \
	"echo \"Booting from mmc${devnum} ...\";" \
	"if load ${devtype} ${devnum}:${distro_bootpart} ${scriptaddr} /boot/boot.scr; then " \
		"source ${scriptaddr};" \
	"fi;\0"

#define BOOTENV_DEV_NAME_ECAM(devtypeu, devtypel, instance) \
	#devtypel #instance " "

#define BOOT_TARGET_DEVICES_ECAM(func) \
	func(ECAM, mmc, 0)

#define BOOT_TARGET_DEVICES(func) \
	BOOT_TARGET_DEVICES_ECAM(func)

#include <config_distro_bootcmd.h>

#define CFG_EXTRA_ENV_SETTINGS \
	MCOM03_COMMON_ENV_SETTINGS \
	BOOTENV

#endif
