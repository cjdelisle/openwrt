# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2006-2025 OpenWrt.org

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

define Image/Build/Initramfs
	$(call Image/Build/$(1))
	cp $(KDIR)/vmlinux-initramfs $(BIN_DIR)/openwrt-$(BOARD)-initramfs.bin
endef

$(eval $(call BuildImage))
