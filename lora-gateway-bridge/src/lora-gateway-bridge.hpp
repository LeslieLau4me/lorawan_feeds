#ifndef _LORA_GATEWAY_BRIDGE_H
#define _LORA_GATEWAY_BRIDGE_H

#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <iostream>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <map>
#include <mosquitto.h>
#include <net/if.h>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <openssl/ssl.h>
#include <pthread.h>
#include <queue>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <toml.hpp>
#include <unistd.h>
#include <vector>
#include "base64.hpp"

#define PROTOCOL_VERSION 2 /* v1.6 */
#define PKT_PUSH_DATA    0
#define PKT_PUSH_ACK     1
#define PKT_PULL_DATA    2
#define PKT_PULL_RESP    3
#define PKT_PULL_ACK     4
#define PKT_TX_ACK       5

#define ETH_NAME_DEFAULT          "eth0"
#define BRIDGE_CONF_DEFAULT       "/etc/lorabridge/lorabridge.toml"
#define BRIDGE_TOPIC_CONF_DEFAULT "/etc/lorabridge/lorabridge_topic.conf"
#define STATUS_SIZE               200
#define NB_PKT_MAX                255 /* max number of packets per fetch/send cycle */
#define TX_BUFF_SIZE              ((540 * NB_PKT_MAX) + 30 + STATUS_SIZE)
#define MAX_LORA_MAC              6
#define MAX_GATEWAY_ID            16
#define LORAWAN_UDP_SERVER        "0.0.0.0"
#define LORAWAN_UDP_PORT          1700

#define MQTT_BROKER_DEFAULT    "127.0.0.1"
#define MQTT_PORT_DEFAULT      1883
#define MQTT_KEEPALIVE_DEFAULT 60

#endif