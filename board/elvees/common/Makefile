#
# Copyright 2015-2016 ELVEES NeoTek JSC
# Copyright 2017 RnD Center "ELVEES", OJSC
#
# SPDX-License-Identifier:	GPL-2.0+
#

obj-y += board.o

ifneq ($(CONFIG_SPL_BUILD),y)
	obj-y += cmd-ddrctl.o
endif
