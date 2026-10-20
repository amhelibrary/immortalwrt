#
# Copyright (C) 2008-2014 The LuCI Team <luci@lists.subsignal.org>
# Copyright (C) 2026 MediaTek Inc.
#
# This is free software, licensed under the Apache License, Version 2.0 .
#

include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-firewall-mtk
LUCI_TITLE:=Firewall and port forwarding application (MTK HNAT support)
# This package ships the very same files as luci-app-firewall (plus the MTK
# HNAT support), so the two applications and their translations are mutually
# exclusive.
#
# It provides the "luci-app-firewall" virtual name so that the collections
# (luci-light, luci-nginx, luci-ssl*, ...) resolve their "+luci-app-firewall"
# dependency through kconfig's provider logic.  Without it those collections
# hard-select the vanilla package, and a "select" cannot be overruled by any
# depends/CONFLICTS statement on this package.
#
# The exclusion itself is declared on the vanilla side
# (CONFLICTS:=luci-app-firewall-mtk in feeds/luci/applications/luci-app-firewall
# and CONFLICTS:=luci-i18n-firewall-mtk-* in its translations).  Declaring it
# here instead would make the parser extend the condition to the selecting
# collection package and trip kconfig's recursive dependency detection.
LUCI_DEPENDS:=+luci-base +uci-firewall
PKG_PROVIDES:=luci-app-firewall
LUCI_PKGARCH:=all

PKG_LICENSE:=Apache-2.0
PKG_MAINTAINER:=ImmortalWrt

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature
