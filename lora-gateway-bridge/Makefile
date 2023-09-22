#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=lora-gateway-bridge

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
  DEPENDS:=+libstdcpp +libevent2 +libmosquitto
  TITLE:=LoRa gateway bridge by C++.
endef

define Package/$(PKG_NAME)/description
  Package for sx1302_hal's udp to mqtt by C++.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lora-gateway-bridge $(1)/usr/bin

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DATA) ./files/etc/init.d/lora-gateway-bridge $(1)/etc/init.d/lora-gateway-bridge

	chmod +x $(1)/etc/init.d/lora-gateway-bridge
        $(INSTALL_DIR) $(1)/etc/lorabridge
        $(INSTALL_DATA) ./files/lorabridge.toml $(1)/etc/lorabridge/
endef

define Package/$(PKG_NAME)/postinst
	#!/bin/sh
        # check if we are on real system
       if [ -z "$${IPKG_INSTROOT}" ]; then
               echo "Enabling rc.d symlink for lora-gateway-bridge"
              chmod 777 /etc/init.d/lora-gateway-bridge
             /etc/init.d/lora-gateway-bridge enable
       fi
      exit 0
endef

define Package/$(PKG_NAME)/prerm
	#!/bin/sh
        # check if we are on real system
       if [ -z "$${IPKG_INSTROOT}" ]; then
               echo "Removing rc.d symlink for lora-gateway-bridge"
                /etc/init.d/lora-gateway-bridge disable
        fi
        exit 0
endef

$(eval $(call BuildPackage,$(PKG_NAME)))