# SPDX-License-Identifier: GPL-2.0
#
# Copyright (C) 2026 MediaTek Inc.
#
# Author: Weijie Gao <weijie.gao@mediatek.com>
#

include $(TOPDIR)/rules.mk

PKG_NAME:=bspconf
PKG_RELEASE:=1

PKG_MAINTAINER:=Weijie Gao <weijie.gao@mediatek.com>

PKG_BUILD_DEPENDS:=fstools

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/bspconf
  SECTION:=MTK Properties
  CATEGORY:=MTK Properties
  SUBMENU:=Applications
  DEPENDS:=+libblkid
  TITLE:=Tool for editing bspconf data used for Dual-Image/Dual-FIP
endef

define Package/bspconf/description
  This tool reads bspconf configuration from device tree passed by u-boot and
  then provides user interface for editing dedicated bspconf items which are
  used to control Dual-Image/Dual-FIP booting behavior.
endef

define Package/bspconf/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bspconf $(1)/usr/sbin
endef

$(eval $(call BuildPackage,bspconf))
