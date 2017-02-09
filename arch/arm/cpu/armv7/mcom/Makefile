#
# (C) Copyright 2000-2003
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# Copyright 2015-2016 ELVEES NeoTek JSC
# Copyright 2017 RnD Center "ELVEES", JSC
# Vasiliy Zasukhin <vzasukhin@elvees.com>
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y += timer.o board.o bootrom.o clock_manager.o
obj-$(CONFIG_SPL_BUILD) += ddr.o

ifeq ($(CONFIG_SPL_BUILD)$(CONFIG_DDR_CALIBRATION),yy)
	obj-y += ddr-calibration.o
endif
