include $(TOPDIR)/rules.mk

PKG_NAME:=chirpstack-gateway-bridge
PKG_VERSION:=4.0.6
PKG_RELEASE:=1

PKG_SOURCE_URL:=https://codeload.github.com/brocaar/chirpstack-gateway-bridge/tar.gz/v$(PKG_VERSION)?
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz

PKG_HASH:=22e8865745893f25e426b281663bb451600bea325fd2239764a171af00a79435
PKG_MAINTAINER:=Jacky Guo <leickwell@gmail.com>
PKG_LICENSE:MIT
PKG_LICENSE_FILES:=LICENSE

PKG_BUILD_DEPENDS:=golang/host
PKG_BUILD_PARALLEL:=1
PKG_USE_MIPS16:=0

GO_PKG:=github.com/brocaar/chirpstack-gateway-bridge
GO_PKG_BUILD_PKG:=github.com/brocaar/chirpstack-gateway-bridge/cmd/chirpstack-gateway-bridge

include $(INCLUDE_DIR)/package.mk
# golang-package.mk should be relocate regarding the env
include $(INCLUDE_DIR)/../feeds/packages/lang/golang/golang-package.mk

define Package/chirpstack-gateway-bridge
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=LoRaWAN
  TITLE:=Chirpstack Gateway Bridge
  DEPENDS:=$(GO_ARCH_DEPENDS) +mosquitto-ssl
  PKGARCH:=all
endef

define Package/chirpstack-gateway-bridge/description
  ChirpStack Gateway Bridge is a service which converts LoRa packet-forwarder 
  protocols into a ChirpStack Network Server common protocol (JSON and Protobuf).
endef

GO_PKG_LDFLAGS:=-s -w
GO_PKG_LDFLAGS_X:=main.version=$(PKG_VERSION)

define Package/chirpstack-gateway-bridge/install
	$(call GoPackage/Package/Install/Bin,$(1))
	mv $(1)/usr/bin/chirpstack-gateway-bridge $(1)/usr/bin/lorabridge

	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_DATA) ./files/lorabridge.init $(1)/etc/init.d/lorabridge
	chmod +x $(1)/etc/init.d/lorabridge

	$(INSTALL_DIR) $(1)/etc/lorabridge
	$(INSTALL_DATA) ./files/lorabridge.toml $(1)/etc/lorabridge/
endef

$(eval $(call GoBinPackage,chirpstack-gateway-bridge))
$(eval $(call BuildPackage,chirpstack-gateway-bridge))
