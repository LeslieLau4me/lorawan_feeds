#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=bridge-pub-test

PKG_BUILD_DEPENDS:=nlohmannjson toml11

PKG_RELEASE:=1.0
PKG_LICENSE:=GPLv3
PKG_MAINTAINER:=liusheng <sheng.liu@minew.com>
PKG_BUILD_DEPENDS:=
include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=LoRaWAN
  DEPENDS:=+libstdcpp +libmosquitto
  TITLE:=lorabridge mqtt publish test.
endef

define Package/$(PKG_NAME)/description
  Package for lorabridge mqtt publish test.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
        $(MAKE) -C $(PKG_BUILD_DIR)/
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bridge-pub-test $(1)/usr/bin
endef

$(eval $(call BuildPackage,$(PKG_NAME)))