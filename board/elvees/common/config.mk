# SPDX-License-Identifier: GPL-2.0+
#
# Copyright 2025 RnD Center "ELVEES", JSC
#

IMG_ALIGN ?= 16

INPUTS-y += u-boot-aligned dtbs-aligned
PHONY += u-boot-aligned dtbs-aligned

quiet_cmd_elveesalign = ALIGN   $2
      cmd_elveesalign = truncate -s %$(IMG_ALIGN) $2

dtbs-aligned: dtbs FORCE
	$(foreach i, $(shell find arch/$(ARCH)/dts/ -name \*.dtb), \
		$(call if_changed,elveesalign,$(i)))

u-boot-aligned: u-boot.bin FORCE
	$(call if_changed,elveesalign,$<)
