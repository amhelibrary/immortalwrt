# SPDX-License-Identifier: GPL-2.0-only
#
# MediaTek Hardware QoS (HQoS) & LuCI Web Controller
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

define Package/luci-app-mtk-hqos
  SECTION:=luci
  CATEGORY:=LuCI
  SUBMENU:=3. Applications
  TITLE:=LuCI support for MediaTek Hardware QoS
  DEPENDS:=+mtk-hqos +luci-base
  PKGARCH:=all
endef

define Package/luci-app-mtk-hqos/description
  LuCI Web user interface for MediaTek Hardware QoS (HQoS) rate limiting service.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
endef

define Build/Compile
	if [ -x $(STAGING_DIR_HOSTPKG)/bin/po2lmo ]; then \
		[ -f ./po/zh_Hans/mtk-hqos.po ] && $(STAGING_DIR_HOSTPKG)/bin/po2lmo ./po/zh_Hans/mtk-hqos.po ./po/zh_Hans/mtk-hqos.zh-cn.lmo; \
		[ -f ./po/zh_Hant/mtk-hqos.po ] && $(STAGING_DIR_HOSTPKG)/bin/po2lmo ./po/zh_Hant/mtk-hqos.po ./po/zh_Hant/mtk-hqos.zh-tw.lmo; \
	fi
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

define Package/luci-app-mtk-hqos/install
	$(INSTALL_DIR) $(1)/www/luci-static/resources/view
	$(INSTALL_DATA) ./luasrc/resources/view/mtk_hqos.js $(1)/www/luci-static/resources/view/mtk_hqos.js
	$(INSTALL_DIR) $(1)/usr/share/luci/menu.d
	$(INSTALL_DATA) ./luasrc/menu.d/luci-app-mtk-hqos.json $(1)/usr/share/luci/menu.d/luci-app-mtk-hqos.json
	$(INSTALL_DIR) $(1)/usr/share/rpcd/acl.d
	$(INSTALL_DATA) ./luasrc/acl.d/luci-app-mtk-hqos.json $(1)/usr/share/rpcd/acl.d/luci-app-mtk-hqos.json
	$(INSTALL_DIR) $(1)/usr/lib/lua/luci/i18n
	[ -f ./po/zh_Hans/mtk-hqos.zh-cn.lmo ] && $(INSTALL_DATA) ./po/zh_Hans/mtk-hqos.zh-cn.lmo $(1)/usr/lib/lua/luci/i18n/mtk-hqos.zh-cn.lmo || true
	[ -f ./po/zh_Hant/mtk-hqos.zh-tw.lmo ] && $(INSTALL_DATA) ./po/zh_Hant/mtk-hqos.zh-tw.lmo $(1)/usr/lib/lua/luci/i18n/mtk-hqos.zh-tw.lmo || true
endef

$(eval $(call BuildPackage,mtk-hqos))
$(eval $(call BuildPackage,luci-app-mtk-hqos))
