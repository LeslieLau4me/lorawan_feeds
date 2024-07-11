#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=sx1302_hal_wrt
PKG_VERSION:=2.1.2
PKG_RELEASE:=1
PKG_LICENSE:=GPLv3
PKG_MAINTAINER:=Minew

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/sx1302_hal_wrt
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=LoRaWAN
  TITLE:=SX1302/SX1303 HAL
  DEPENDS:=+kmod-spi-dev +liburcu
  MENU:=1
endef

define Package/sx1302_hal_wrt/description
  Utilities for SX1302/SX1303 Hardware Abstraction Layer
endef

define Package/sx1302_hal_wrt/config
    source "$(SOURCE)/Config.in"
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	rsync -avh --delete --exclude=".git**" ./lora-sx1302-hal-src/* $(PKG_BUILD_DIR)/
	$(CP) ./files/* $(PKG_BUILD_DIR)/
	$(call Build/Prepare/Default)
endef

CFLAGS = $(TARGET_CFLAGS)

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/libtools \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(ARCH)"

	$(MAKE) -C $(PKG_BUILD_DIR)/libloragw \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(ARCH)"

	$(MAKE) -C $(PKG_BUILD_DIR)/util_boot \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(ARCH)"

	$(MAKE) -C $(PKG_BUILD_DIR)/util_chip_id \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(ARCH)"

	$(MAKE) -C $(PKG_BUILD_DIR)/util_net_downlink \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(ARCH)"

	$(MAKE) -C $(PKG_BUILD_DIR)/util_spectral_scan \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(ARCH)"

	$(MAKE) -C $(PKG_BUILD_DIR)/packet_forwarder \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		ARCH="$(ARCH)"
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/lgw
	$(CP) $(PKG_BUILD_DIR)/libloragw/inc/loragw_* $(1)/usr/include/lgw
	$(CP) $(PKG_BUILD_DIR)/libloragw/inc/config.h $(1)/usr/include/lgw
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libloragw/libloragw.a $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/libtools/libtinymt32.a $(1)/usr/lib/
endef

define Package/sx1302_hal_wrt/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/packet_forwarder/lora_pkt_fwd $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/libloragw/test_loragw_* $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/util_chip_id/chip_id $(1)/usr/bin

	$(INSTALL_DIR) $(1)/etc/lora_pkt_fwd
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/packet_forwarder/global_conf.json.sx1250.* $(1)/etc/lora_pkt_fwd
ifeq ($(CONFIG_LORA_SX1302_HAL_US915),y)
ifeq ($(CONFIG_LORA_SX1302_HAL_GPS),y)
	cp $(1)/etc/lora_pkt_fwd/global_conf.json.sx1250.US915 $(1)/etc/lora_pkt_fwd/global_conf.json
else
	cp $(1)/etc/lora_pkt_fwd/global_conf.json.sx1250.US915.no_gps $(1)/etc/lora_pkt_fwd/global_conf.json
endif
else ifeq ($(CONFIG_LORA_SX1302_HAL_EU868),y)
ifeq ($(CONFIG_LORA_SX1302_HAL_GPS),y)
	cp $(1)/etc/lora_pkt_fwd/global_conf.json.sx1250.EU868 $(1)/etc/lora_pkt_fwd/global_conf.json
else
	cp $(1)/etc/lora_pkt_fwd/global_conf.json.sx1250.EU868.no_gps $(1)/etc/lora_pkt_fwd/global_conf.json
endif
else ifeq ($(CONFIG_LORA_SX1302_HAL_CN490),y)
ifeq ($(CONFIG_LORA_SX1302_HAL_GPS),y)
	cp $(1)/etc/lora_pkt_fwd/global_conf.json.sx1250.CN490 $(1)/etc/lora_pkt_fwd/global_conf.json
else
	cp $(1)/etc/lora_pkt_fwd/global_conf.json.sx1250.CN490.no_gps $(1)/etc/lora_pkt_fwd/global_conf.json
endif
endif
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/tools/reset_lgw.sh $(1)/usr/bin
	chmod +x $(1)/usr/bin/reset_lgw.sh

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DATA) ./files/etc/init.d/packet_forwarder $(1)/etc/init.d/packet_forwarder
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DATA) ./files/usr/bin/led_control.sh $(1)/usr/bin/led_control.sh
	$(INSTALL_DATA) ./files/usr/bin/lte_rst.sh $(1)/usr/bin/lte_rst.sh

	$(INSTALL_DIR) $(1)/etc/lorawan_region
ifeq ($(CONFIG_LORA_SX1302_HAL_US915),y)
	$(INSTALL_DATA) ./files/lorawan_region_us915.conf $(1)/etc/lorawan_region/
else ifeq ($(CONFIG_LORA_SX1302_HAL_EU868),y)
	$(INSTALL_DATA) ./files/lorawan_region_eu868.conf $(1)/etc/lorawan_region/
else ifeq ($(CONFIG_LORA_SX1302_HAL_CN490),y)
	$(INSTALL_DATA) ./files/lorawan_region_cn490.conf $(1)/etc/lorawan_region/
endif
	chmod +x $(1)/etc/init.d/packet_forwarder
	chmod +x $(1)/usr/bin/led_control.sh
	chmod +x $(1)/usr/bin/lte_rst.sh
endef

define Package/sx1302_hal_wrt/postinst
#!/bin/sh
if [ -z "$${IPKG_INSTROOT}" ]; then
	echo "Enable packet_forwarder"
	/etc/init.d/packet_forwarder enable
fi
exit 0
endef

define Package/sx1302_hal_wrt/prerm
#!/bin/sh
if [ -z "$${IPKG_INSTROOT}" ]; then
	echo "Disable packet_forwarder"
	/etc/init.d/packet_forwarder disable
fi
exit 0
endef

$(eval $(call BuildPackage,sx1302_hal_wrt))
