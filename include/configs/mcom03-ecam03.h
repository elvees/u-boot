/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2022 RnD Center "ELVEES", JSC
 *
 * Configuration settings for the ECAM03 boards
 */

#ifndef __MCOM03_ECAM03_H
#define __MCOM03_ECAM03_H

#include "mcom03-common.h"

#define BOOTENV_DEV_ECAM(devtypeu, devtypel, instance) \
	"bootcmd_ecam_mmc" #instance "=" \
	"env export -t ${loadaddr};" \
	"env append -0x30000;" \
	"devtype=mmc;" \
	"devnum=" #instance "; " \
	"if test \"${bootvol}\" = \"b\"; then " \
		"distro_bootpart=2;" \
	"else " \
		"distro_bootpart=1;" \
	"fi;" \
	"env import -d -t ${loadaddr};" \
	"echo \"Booting from mmc${devnum} ...\";" \
	"if load ${devtype} ${devnum}:${distro_bootpart} ${scriptaddr} /boot/boot.scr; then " \
		"source ${scriptaddr};" \
	"fi;\0"

#define BOOTENV_DEV_NAME_ECAM(devtypeu, devtypel, instance) \
	"ecam_mmc" #instance " "

#define BOOT_TARGET_DEVICES_ECAM(func) \
	func(ECAM, ecam_mmc, 1) \
	func(ECAM, ecam_mmc, 0)

#define BOOT_TARGET_DEVICES(func) \
	BOOT_TARGET_DEVICES_ECAM(func)

#include <config_distro_bootcmd.h>

#define CONFIG_EXTRA_ENV_SETTINGS \
	MCOM03_COMMON_ENV_SETTINGS \
	BOOTENV

#endif
