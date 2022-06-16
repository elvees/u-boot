/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2022 RnD Center "ELVEES", JSC
 *
 * Configuration settings for the ECAM03 boards
 */

#ifndef __MCOM03_ECAM03_H
#define __MCOM03_ECAM03_H

#include "mcom03-common.h"

#define BOOT_TARGET_DEVICES(func)

#include <config_distro_bootcmd.h>

#define CONFIG_EXTRA_ENV_SETTINGS \
	MCOM03_COMMON_ENV_SETTINGS \
	BOOTENV

#endif
