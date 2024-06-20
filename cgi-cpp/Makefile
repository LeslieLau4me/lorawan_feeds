#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=cgi-cpp

PKG_RELEASE:=1.0
PKG_LICENSE:=GPLv3
PKG_MAINTAINER:=Minew

PKG_BUILD_DEPENDS:=nlohmannjson toml11 easyloggingpp

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/$(PKG_NAME)
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=LoRaWAN
  DEPENDS:=$(DRV_DEPENDS) +libstdcpp
  TITLE:=CGI utility for $(PKG_NAME)
endef

define Package/$(PKG_NAME)/description
  This package contains an cgi utility that is useful for configure the gateway's settings which writen in C++.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
	#$(CP) ./files/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += $(if $(CONFIG_CGIC_DEBUG), -ggdb3)
CMAKE_OPTIONS += $(if $(CONFIG_CGIC_DEBUG), -DCMAKE_BUILD_TYPE=Debug)

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/logs
	$(INSTALL_DATA) ./files/cgicpp_log.conf $(1)/etc/logs/

	$(INSTALL_DIR) $(1)/etc/lorawan_filter
	$(INSTALL_DATA) ./files/lorawan_filter.conf $(1)/etc/lorawan_filter/

	$(INSTALL_DIR) $(1)/etc/lorawan_region
	$(INSTALL_DATA) ./files/lorawan_region.conf $(1)/etc/lorawan_region/

	$(INSTALL_DIR) $(1)/etc/lorawan_workmode
	$(INSTALL_DATA) ./files/lorawan_workmode.conf $(1)/etc/lorawan_workmode/

	$(INSTALL_DIR) $(1)/etc/notification
	$(INSTALL_DATA) ./files/notification.conf $(1)/etc/notification/

	$(INSTALL_DIR) $(1)/etc/lte_usb
	$(INSTALL_DATA) ./files/lte_usb.conf $(1)/etc/lte_usb/

	$(INSTALL_DIR) $(1)/etc/lorabridge
	$(INSTALL_DATA) ./files/lorabridge_topic.conf $(1)/etc/lorabridge/

	$(INSTALL_DIR) $(1)/etc/lorawan_scripts
	$(INSTALL_DATA) ./files/mode.sh $(1)/etc/lorawan_scripts/lorawan_mode
	chmod +x $(1)/etc/lorawan_scripts/lorawan_mode

	$(INSTALL_DIR) $(1)/usr/libexec $(1)/www/cgi-bin/ $(1)/etc/config/thingoo
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cgi-cpp $(1)/usr/libexec/

	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-toml-general-get
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-toml-filters-get
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-toml-backend-get
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-toml-integration-get
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-toml-metrics-get
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-toml-meta-data-get
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-toml-commands-get
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-toml-all-get


	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-toml-general-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-toml-filters-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-toml-backend-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-toml-integration-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-toml-metrics-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-toml-meta-data-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-toml-commands-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-toml-all-set

	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-bs-radio-common-get
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-bs-radio0-get
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-bs-radio1-get
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-bs-station-get
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-bs-all-get

	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-bs-radio-common-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-bs-radio0-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-bs-radio1-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-bs-station-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-bs-tc-uri-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-bs-tc-key-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-bs-cups-uri-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-bs-cups-key-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-bs-all-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-lorawanfilter-all-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-lorawanfilter-all-get

	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-lorawanregion-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-lorawanregion-get

	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-workmode-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-workmode-get

	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-online-get
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-lorawanframe-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-lorawanframe-get
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-notification-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-notification-get
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-bridgetopic-set
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-bridgetopic-get
	$(LN) ../../usr/libexec/cgi-cpp $(1)/www/cgi-bin/cgicpp-lte-get

endef

$(eval $(call BuildPackage,$(PKG_NAME)))
