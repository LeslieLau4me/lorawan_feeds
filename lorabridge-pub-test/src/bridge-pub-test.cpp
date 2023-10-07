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
#include <nlohmann/json.hpp>
#define SERVER_IP_ADDR "127.0.0.1"
#define MAX_LORA_MAC   6
#define MAX_GATEWAY_ID 16

using namespace std;
using json = nlohmann::json;
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

    topic_sub_txpk = string("gateway/") + string(gateway_eui) + string("/event/") + string("tx");
    std::cout << "Tx topic receiving tx packet:" << topic_sub_txpk << std::endl;

    return 0;
}

void catch_signal(int num)
{
    printf("input signal %d\n", num);
    mosquitto_lib_cleanup();
    exit(EXIT_SUCCESS);
}
void on_connect_publish(struct mosquitto *mosq, void *obj, int reason_code)
{
	/* 打印出连接结果。 mosquitto_connect string() 为 MQTT v3.x 客户端生成适当的字符串，*/
	printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
	if (reason_code != MOSQ_ERR_SUCCESS) {
        /* 如果连接因任何原因失败，我们不想继续,没有这个，客户端将尝试重新连接。 */
		mosquitto_disconnect(mosq);
	}
}

void on_publish(struct mosquitto *mosq, void *obj, int mid)
{
	printf("Message with mid %d has been published.\n", mid);
}

void publish_tx_data(struct mosquitto *mosq)
{
	int rc = -1;
    json tx_json;
    tx_json["txpk"]["imme"] = true;
    tx_json["txpk"]["freq"] = 912.3;
    tx_json["txpk"]["rfch"] = 0;
    tx_json["txpk"]["powe"] = 12;
    tx_json["txpk"]["modu"] = "LORA";
    tx_json["txpk"]["datr"] = "SF11BW125";
    tx_json["txpk"]["codr"] = "4/6";
    tx_json["txpk"]["ipol"] = false;
    tx_json["txpk"]["size"] = 32;
    tx_json["txpk"]["data"] = "H3P3N2i9qc4yt7rK7ldqoeCVJGBybzPY5h1Dd7P7p8v";
    string tx_string = tx_json.dump();
	rc = mosquitto_publish(mosq, NULL, topic_sub_txpk.c_str(), tx_string.length(), tx_string.c_str(), 0, false);
	if(rc != MOSQ_ERR_SUCCESS){
		fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
	}
}

int main(void)
{
    if (lora_BRIDGE_TOPIC_SET_mqtt_topic() < 0) {
        std::cerr << "Failed to setup mqtt topic." << std::endl;
        return -1;
    }
    signal(SIGINT, catch_signal);
	signal(SIGSTOP, catch_signal);
    mosquitto_lib_init();
    struct mosquitto *mosq = NULL;
    int ret = -1;
    mosq =  mosquitto_new(NULL, true, NULL);
    if (mosq == NULL) {
        perror("mqtt create failed");
        return -1;
    }
    mosquitto_connect_callback_set(mosq, on_connect_publish);
    mosquitto_publish_callback_set(mosq, on_publish);
    ret = mosquitto_connect(mosq, SERVER_IP_ADDR, 1883, 60);
    if (ret != MOSQ_ERR_SUCCESS) {
        mosquitto_destroy(mosq);
        fprintf(stderr, "Error: %s\n", mosquitto_strerror(ret));
        return -1;
    }

    ret = mosquitto_loop_start(mosq);
    if (ret != MOSQ_ERR_SUCCESS) {
        mosquitto_destroy(mosq);
        fprintf(stderr, "Error: %s\n", mosquitto_strerror(ret));
        return -1;
    }
    while (1) {
        publish_tx_data(mosq);
        sleep(1);
    }
}