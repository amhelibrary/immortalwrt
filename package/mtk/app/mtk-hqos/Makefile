# SPDX-License-Identifier: GPL-2.0-only
#
# MediaTek Hardware QoS (HQoS) Rate Limiter
# Copyright (C) 2026 ImmortalWrt
#

include $(TOPDIR)/rules.mk

PKG_NAME:=mtk-hqos
PKG_VERSION:=1.0
PKG_RELEASE:=6

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)
PKG_CONFIG_DEPENDS:=

include $(INCLUDE_DIR)/package.mk

define Package/mtk-hqos
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Routing and Redirection
  TITLE:=MediaTek Hardware QoS (HQoS) Rate Limiter
  DEPENDS:=+kmod-mediatek_hnat +kmod-br-netfilter
endef

define Package/mtk-hqos/description
  Hardware-accelerated QoS and IP/MAC bandwidth rate limiter using MediaTek QDMA and HNAT/PPE.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
endef

define Build/Compile
endef

define Package/mtk-hqos/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) ./files/mtk-hqos $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/mtk-hqos.config $(1)/etc/config/mtk-hqos
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/mtk-hqos.init $(1)/etc/init.d/mtk-hqos
	$(INSTALL_DIR) $(1)/etc/hotplug.d/dhcp
	$(INSTALL_BIN) ./files/mtk-hqos.hotplug $(1)/etc/hotplug.d/dhcp/99-mtk-hqos
endef

$(eval $(call BuildPackage,mtk-hqos))
