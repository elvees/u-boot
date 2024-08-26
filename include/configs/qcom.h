/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration file for Qualcomm Snapdragon boards
 *
 * (C) Copyright 2021 Dzmitry Sankouski <dsankouski@gmail.com>
 * (C) Copyright 2023 Linaro Ltd.
 */

#ifndef __CONFIGS_SNAPDRAGON_H
#define __CONFIGS_SNAPDRAGON_H

#define CFG_SYS_BAUDRATE_TABLE	{ 115200, 230400, 460800, 921600 }

/* Load addressed are calculated during board_late_init(). See arm/mach-snapdragon/board.c */
#define CFG_EXTRA_ENV_SETTINGS \
	"stdin=serial,button-kbd\0"	\
	"stdout=serial,vidconsole\0"	\
	"stderr=serial,vidconsole\0" \
	"bootcmd=bootm $prevbl_initrd_start_addr\0"

#endif
