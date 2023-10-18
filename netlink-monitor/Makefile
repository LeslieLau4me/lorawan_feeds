#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=netlink-monitor

PKG_RELEASE:=1.0
PKG_LICENSE:=GPLv3
PKG_MAINTAINER:=Minew
PKG_BUILD_DEPENDS:=
include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=LoRaWAN
  DEPENDS:=+libevent2
  TITLE:=Netlink monitor
endef

define Package/$(PKG_NAME)/description
  This package used Netlink monitor
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/netlink-monitor $(1)/usr/bin

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DATA) ./files/etc/init.d/network-monitor $(1)/etc/init.d/netlink-monitor

	chmod +x $(1)/etc/init.d/netlink-monitor
endef

define Package/$(PKG_NAME)/postinst
	#!/bin/sh
        # check if we are on real system
       if [ -z "$${IPKG_INSTROOT}" ]; then
               echo "Enabling rc.d symlink for netlink-monitor"
              chmod 777 /etc/init.d/netlink-monitor
             /etc/init.d/netlink-monitor enable
       fi
      exit 0
endef

define Package/$(PKG_NAME)/prerm
	#!/bin/sh
        # check if we are on real system
       if [ -z "$${IPKG_INSTROOT}" ]; then
               echo "Removing rc.d symlink for netlink-monitor"
                /etc/init.d/netlink-monitor disable
        fi
        exit 0
endef

$(eval $(call BuildPackage,$(PKG_NAME)))