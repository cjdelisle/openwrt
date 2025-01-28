include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

define Target/Description
	Build firmware images for EN75xx MIPS based boards.
endef

# define Device/Default
# 	PROFILES = Bare Kernel $$(DEVICE_NAME)
# 	KERNEL_NAME := Image
# 	KERNEL = kernel-bin | append-dtb
# 	DEVICE_DTS_DIR := $(DTS_DIR)
# 	IMAGES := kernel.bin
# 	IMAGE/kernel.bin := append-kernel
# endef

# define Image/Build
# 	$(call Image/Build/$(1),$(1))
# endef

# define Image/Build/Initramfs
# 	$(call Image/Build/$(1))
# 	cp $(KDIR)/vmlinux-initramfs $(BIN_DIR)/openwrt-$(BOARD)-initramfs.bin
# endef

# define Image/Build/Prepare
# 	cp $(KDIR)/vmlinux $(BIN_DIR)/openwrt-$(BOARD)-barekernel.bin
# endef

# define Image/Build/Initramfs
# 	$(call Image/Build/$(1))
# 	cp $(KDIR)/vmlinux $(BIN_DIR)/openwrt-$(BOARD)-barekernel.bin
# endef

#  PROFILES := Default

define Device/en751221_bare_kernel
  KERNEL_NAME := vmlinux
  KERNEL := kernel-bin
  IMAGES := linux.bin
  IMAGE/linux.bin := kernel-bin | append-dts
  DEVICE_VENDOR := Generic
  DEVICE_MODEL := EN751221 Bare Kernel
  DEVICE_DTS := en751221-generic
  DEVICE_DTS_DIR := ../dts
endef

# define Device/en751221_bare_kernel
# 	DEVICE_VENDOR := Generic
# 	DEVICE_MODEL := EN751221 Bare Kernel
# 	DEVICE_DTS := en751221-generic
# 	DEVICE_DTS_DIR := ../dts
# 	KERNEL := kernel-bin | append-dtb
# 	DEVICE_DESCRIPTION := Linux kernel and device tree ONLY (for testing). \
# 		No filesystem included.
# endef
TARGET_DEVICES += en751221_bare_kernel

$(eval $(call BuildImage))
