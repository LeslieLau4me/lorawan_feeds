#
# Copyright (C) 2022 TDT AG <development@tdt.de>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
#
include $(TOPDIR)/rules.mk

PKG_NAME:=basicstation
PKG_VERSION:=2.0.7
PKG_RELEASE:=$(AUTORELEASE)

# PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
# PKG_SOURCE_URL:=https://codeload.github.com/lorabasics/basicstation/tar.gz/v$(PKG_VERSION)?
# PKG_HASH:=7e285de94bae1174b4c3496fc3ab15fe67c72f92c0693d2320bafc654a9dfb43

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=ssh://git@gitlab.minew.net:2022/gateway/openwrt/packages/gateway_package_basicstation.git
PKG_SOURCE_VERSION:=9df4c13041b89b4d90a2647bb288bd66134f7bff

PKG_MAINTAINER:=Marcus Schref <mschref@tdt.de>
PKG_LICENSE:=BSD-3-Clause
PKG_LICENSE_FILES:=LICENSE

PKG_BUILD_DEPENDS:=lora-sx1302-hal mbedtls

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/basicstation
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=LoRaWAN
  TITLE:=LoRa Basic Station
  DEPENDS:=+kmod-spi-dev +liburcu
endef

define Package/basicstation/description
  LoRa Basic Station. The LoRaWAN Gateway Software.
endef

define Package/basicstation/conffiles
/etc/config/basicstation
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	rm -rf $(PKG_BUILD_DIR)/deps/
	rm -f $(PKG_BUILD_DIR)/makefile
	rm -f $(PKG_BUILD_DIR)/makefile.s2core
	rm -f $(PKG_BUILD_DIR)/setup.gmk
endef

define Package/basicstation/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/station $(1)/usr/bin
	# $(INSTALL_DIR) $(1)/etc/config
	# $(INSTALL_CONF) ./files/etc/config/basicstation $(1)/etc/config/basicstation
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/basicstation $(1)/etc/init.d/basicstation

	$(INSTALL_DIR) $(1)/etc/basicstation
	#$(INSTALL_BIN) ./files/etc/basicstation/reset_lgw.sh $(1)/etc/basicstation/
	$(INSTALL_BIN) ./files/etc/basicstation/tc.trust $(1)/etc/basicstation/
	$(INSTALL_DATA) ./files/etc/config/station.conf $(1)/etc/basicstation/

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) ./files/usr/bin/rinit.sh $(1)/usr/bin

	#chmod +x $(1)/etc/basicstation/reset_lgw.sh
	chmod +x $(1)/usr/bin/rinit.sh
endef

$(eval $(call BuildPackage,basicstation))
