#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <iostream>
#include <mosquitto.h>
#include <net/if.h>
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
#include <unistd.h>

#define SERVER_IP_ADDR "127.0.0.1"
#define MQTT_TOPIC     "example/temperature"
#define MAX_LORA_MAC   6
#define MAX_GATEWAY_ID 16

using namespace std;

static string topic_pub_rxpk;
static string topic_pub_downlink;
static string topic_pub_downlink_ack;
static string topic_pub_gateway_stat;
static string topic_sub_txpk;

static int get_local_eth_mac(unsigned char *mac_address)
{
    struct ifreq ifr;
    int          sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to socket." << '\n';
        return -1;
    }
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
        std::cerr << "Failed to ioctl." << '\n';
        close(sock);
        return -1;
    }
    unsigned char *buf = (unsigned char *)ifr.ifr_hwaddr.sa_data;
    memcpy(mac_address, buf, MAX_LORA_MAC);
    close(sock);
    return 0;
}

static int generate_gateway_id_by_mac(char *gw_id)
{
    unsigned char mac_buf[MAX_LORA_MAC] = { 0 };
    if (get_local_eth_mac(mac_buf) != 0) {
        return -1;
    }
    char id_buf[MAX_GATEWAY_ID + 1] = { 0 };
    /* clang-format off */
    snprintf(id_buf, MAX_GATEWAY_ID + 1, "%02x%02x%02xfffe%02x%02x%02x", mac_buf[0],
                                mac_buf[1], mac_buf[2], mac_buf[3], mac_buf[4], mac_buf[5]);
    memcpy(gw_id, id_buf, MAX_GATEWAY_ID + 1);
    /* clang-format on */
    return 0;
}

static int lora_BRIDGE_TOPIC_SET_mqtt_topic(void)
{
    char gateway_eui[MAX_GATEWAY_ID + 1] = { 0 };
    if (generate_gateway_id_by_mac(gateway_eui) < 0) {
        std::cerr << "Failed to get eth mac." << std::endl;
        return -1;
    }
    /*
        Notice: 目前主题暂定如下，后续如有需要可配合前端实现修改主题模版
    */
    /* clang-format off */
    topic_pub_rxpk         = string("gateway/") + string(gateway_eui) + string("/event/") + string("up");
    topic_pub_downlink     = string("gateway/") + string(gateway_eui) + string("/event/") + string("down");
    topic_pub_downlink_ack = string("gateway/") + string(gateway_eui) + string("/event/") + string("ack");
    topic_pub_gateway_stat = string("gateway/") + string(gateway_eui) + string("/event/") + string("stat");
    topic_sub_txpk         = string("gateway/") + string(gateway_eui) + string("/event/") + string("tx");
    /* clang-format on */
    std::cout << "Uplink rx topic:" << topic_pub_rxpk << std::endl;
    std::cout << "Downlink tx topic:" << topic_pub_downlink << std::endl;
    std::cout << "Downlink tx ack topic:" << topic_pub_downlink_ack << std::endl;
    std::cout << "Gateway statistics topic:" << topic_pub_gateway_stat << std::endl;
    std::cout << "Tx topic receiving tx packet:" << topic_sub_txpk << std::endl;

    return 0;
}

void catch_signal(int num)
{
    printf("input signal %d\n", num);
    mosquitto_lib_cleanup();
    exit(EXIT_SUCCESS);
}

void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
    printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
    if (reason_code != 0) {
        mosquitto_disconnect(mosq);
        return;
    }

    mosquitto_subscribe(mosq, NULL, topic_pub_rxpk.c_str(), 1);
    mosquitto_subscribe(mosq, NULL, topic_pub_gateway_stat.c_str(), 1);
    mosquitto_subscribe(mosq, NULL, topic_pub_downlink.c_str(), 1);
    mosquitto_subscribe(mosq, NULL, topic_pub_downlink_ack.c_str(), 1);
}

void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
    bool have_subscription = false;
    for (int i = 0; i < qos_count; i++) {
        printf("on_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
        if (granted_qos[i] <= 2) {
            have_subscription = true;
        }
    }
    if (have_subscription == false) {
        fprintf(stderr, "Error: All subscriptions rejected.\n");
        mosquitto_disconnect(mosq);
    }
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    printf("topic:%s qos:%d payload:%s\n", msg->topic, msg->qos, (char *)msg->payload);
}

int main(int argc, char const *argv[])
{
    if (lora_BRIDGE_TOPIC_SET_mqtt_topic() < 0) {
        std::cerr << "Failed to setup mqtt topic." << std::endl;
        return -1;
    }
    signal(SIGINT, catch_signal);
    signal(SIGSTOP, catch_signal);
    mosquitto_lib_init();
    struct mosquitto *mosq = NULL;
    int               ret  = -1;

    mosq = mosquitto_new(NULL, true, NULL);
    if (mosq == NULL) {
        fprintf(stderr, "Error: Out of memory.\n");
        return 1;
    }
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_subscribe_callback_set(mosq, on_subscribe);
    mosquitto_message_callback_set(mosq, on_message);

    ret = mosquitto_connect(mosq, SERVER_IP_ADDR, 1883, 60);
    if (ret != MOSQ_ERR_SUCCESS) {
        mosquitto_destroy(mosq);
        fprintf(stderr, "Error: %s\n", mosquitto_strerror(ret));
        return 1;
    }
    mosquitto_loop_forever(mosq, -1, 1);
    return 0;
}
