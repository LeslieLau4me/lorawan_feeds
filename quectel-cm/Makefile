include $(TOPDIR)/rules.mk

PKG_NAME:=quectel-cm
PKG_VERSION:=1.6.4
PKG_RELEASE:=1

PKG_MAINTAINER:=Minew
PKG_LICENSE:=MIT

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/quectel-cm
	SECTION:=net
	CATEGORY:=Network
	SUBMENU:=LoRaWAN
	TITLE:=Quectel Connect Manager
endef

define Package/quectel-cm/description
	Quectel Connect Manager.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/quectel-cm/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/quectel-CM $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/etc/init.d/quectel $(1)/etc/init.d/quectel
endef

$(eval $(call BuildPackage,quectel-cm))

