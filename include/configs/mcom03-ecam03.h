/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2022 RnD Center "ELVEES", JSC
 *
 * Configuration settings for the ECAM03 boards
 */

#ifndef __MCOM03_ECAM03_H
#define __MCOM03_ECAM03_H

#include "mcom03-common.h"

#ifndef CONFIG_FDTFILE
#define CONFIG_FDTFILE CONFIG_DEFAULT_DEVICE_TREE ".dtb"
#endif

#define EXTRABOOTENV "bootvol=a\0" \
	"fdtfile=" CONFIG_FDTFILE "\0" \
	"safe_bootvol=a\0"

#define BOOTENV_DEV_ECAM(devtypeu, devtypel, instance) "bootcmd_ecam=" \
	"if test \"${bootvol}\" != \"${safe_bootvol}\"; then " \
		"if env exists tried_to_boot; then " \
			"setenv bootvol ${safe_bootvol};" \
			"setenv tried_to_boot;" \
			"saveenv;" \
		"else " \
			"setenv tried_to_boot true;" \
			"saveenv;" \
		"fi;" \
	"fi;" \
	"devtype=mmc;" \
	"test -n \"$devnum\" || devnum=0;" \
	"if test \"${bootvol}\" = \"a\"; then " \
		"distro_bootpart=1;" \
	"else " \
		"distro_bootpart=2;" \
	"fi;" \
	"load ${devtype} ${devnum}:${distro_bootpart} ${scriptaddr} /boot/boot.scr; " \
	"source ${scriptaddr}\0"

#define BOOTENV_DEV_NAME_ECAM(devtypeu, devtypel, instance) "ecam "

#define BOOT_TARGET_DEVICES_ECAM(func) func(ECAM, ecam, na)

#define BOOT_TARGET_DEVICES(func) \
	BOOT_TARGET_DEVICES_ECAM(func)

#include <config_distro_bootcmd.h>

#define CONFIG_EXTRA_ENV_SETTINGS \
	MCOM03_COMMON_ENV_SETTINGS \
	BOOTENV \
	EXTRABOOTENV

#endif
