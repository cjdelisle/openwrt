# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2009-2025 OpenWrt.org

include $(TOPDIR)/rules.mk

ARCH:=mips
BOARD:=en75
BOARDNAME:=Econet EN75xx MIPS
FEATURES:=dt source-only
CPU_TYPE:=24kc
#SUBTARGETS:=en7526

KERNEL_PATCHVER:=6.6

define Target/Description
	Build firmware image for Econet EN75xx MIPS based boards.
endef

# include the profiles
include $(INCLUDE_DIR)/target.mk

#DEFAULT_PACKAGES+=kmod-ensoc_eth kmod-mips-wdt kmod-mt7603 kmod-mt76x2

$(eval $(call BuildTarget))