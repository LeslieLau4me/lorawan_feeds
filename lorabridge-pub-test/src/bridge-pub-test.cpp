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
#include <toml.hpp>
#define SERVER_IP_ADDR "127.0.0.1"
#define MAX_LORA_MAC   6
#define MAX_GATEWAY_ID 16
#define BRIDGE_CONF_DEFAULT       "/etc/lorabridge/lorabridge.toml"
#define BRIDGE_TOPIC_CONF_DEFAULT "/etc/lorabridge/lorabridge_topic.conf"
using namespace std;
using json = nlohmann::json;
static string topic_sub_txpk;

static int mqtt_keepalive = 60;

static int     mqtt_port;
static string  mqtt_host;
static string  mqtt_username;
static string  mqtt_password;
static string  ca_file_path;
static string  cert_file_path;
static string  key_file_path;
static string  client_id;
static uint8_t mqtt_qos;
static bool    mqtt_clean_session;

static double tx_freq = 0.0;

class BridgeToml
{
  private:
    toml::value toml_data;

    string backend_type;
    // backend.semtech_udp
    string   udp_ip;
    uint32_t udp_port = 0;
    bool     skip_crc_check;
    bool     fake_rx_time;

    // integration.mqtt
    string   event_topic_template;
    string   command_topic_template;
    uint32_t max_reconnect_interval = 0;

    // integration.mqtt.auth
    string mqtt_auth_type;
    // integration.mqtt.auth.generic
    string   generic_ip;
    uint32_t generic_port = 0;
    string   generic_username;
    string   generic_password;
    uint8_t  generic_qos = 0;
    bool     generic_clean_session;
    string   generic_client_id;
    string   generic_ca_cert;
    string   generic_tls_cert;
    string   generic_tls_key;

    void parse_toml_backend_udp(void);
    void parse_toml_integration_generic(void);
    void parse_local_for_each(void);

  public:
    BridgeToml();
    ~BridgeToml();

    void get_bridge_config_info(void);
};

BridgeToml::BridgeToml() {}
BridgeToml::~BridgeToml() {}

void BridgeToml::get_bridge_config_info(void)
{
    this->toml_data = toml::parse<toml::discard_comments>(BRIDGE_CONF_DEFAULT);
    this->parse_local_for_each();
    mqtt_host          = this->generic_ip;
    mqtt_port          = this->generic_port;
    ca_file_path       = this->generic_ca_cert;
    cert_file_path     = this->generic_tls_cert;
    key_file_path      = this->generic_tls_key;
    client_id          = this->generic_client_id;
    mqtt_username      = this->generic_username;
    mqtt_password      = this->generic_password;
    mqtt_qos           = this->generic_qos;
    mqtt_clean_session = this->generic_clean_session;
}

void BridgeToml::parse_toml_backend_udp(void)
{
    const auto &backend = toml::find(toml_data, "backend");
    this->backend_type  = toml::find<std::string>(backend, "type");

    const auto &semtech_udp = toml::find(backend, "semtech_udp");
    string      udp_bind    = toml::find<std::string>(semtech_udp, "udp_bind");
    std::cout << "[Bridge]udp bind:" << udp_bind << std::endl;
    auto  idx     = udp_bind.find(":");
    char *ip_port = const_cast<char *>(udp_bind.c_str());
    if (ip_port != NULL && idx > 0) {
        char *ip       = strtok(ip_port, ":");
        this->udp_ip   = string(ip);
        char *port     = strtok(NULL, ":");
        this->udp_port = atoi(port);
    }
    this->skip_crc_check = toml::find<bool>(semtech_udp, "skip_crc_check");
    this->fake_rx_time   = toml::find<bool>(semtech_udp, "fake_rx_time");
}

void BridgeToml::parse_toml_integration_generic(void)
{
    // 定位到integration.mqtt
    const auto &integration      = toml::find(this->toml_data, "integration");
    const auto &mqtt             = toml::find(integration, "mqtt");
    this->event_topic_template   = toml::find<std::string>(mqtt, "event_topic_template");
    this->command_topic_template = toml::find<std::string>(mqtt, "command_topic_template");
    const auto &auth             = toml::find(mqtt, "auth");
    this->mqtt_auth_type         = toml::find<std::string>(auth, "type");
    const auto generic           = toml::find(auth, "generic");
    string     bind              = toml::find<std::string>(generic, "server");
    std::cout << "[Bridge]mqtt generic bind: " << bind << std::endl;
    auto   idx     = bind.find(":");
    char  *ip_port = const_cast<char *>(bind.c_str());
    string actual_ip;
    if (ip_port != NULL && idx > 0) {
        char *ip = strtok(ip_port, ":");
        // tcp/ssl/http类型
        if (strlen(ip) < strlen("0.0.0.0")) {
            char *ip_part    = strtok(NULL, ":");
            actual_ip        = string(ip) + string(":") + string(ip_part);
            this->generic_ip = actual_ip;
        } else {
            this->generic_ip = string(ip);
        }
        char *port         = strtok(NULL, ":");
        this->generic_port = atoi(port);
        std::cout << "[Bridge]mqtt generic the port: " << port << std::endl;
    }
    std::cout << "[Bridge]mqtt generic the server: " << this->generic_ip << std::endl;
    this->generic_username = toml::find<std::string>(generic, "username");
    std::cout << "[Bridge]mqtt generic the username: " << this->generic_username << std::endl;
    this->generic_password = toml::find<std::string>(generic, "password");
    std::cout << "[Bridge]mqtt generic the password: " << this->generic_password << std::endl;
    this->generic_qos = toml::find<std::uint32_t>(generic, "qos");
    std::cout << "[Bridge]mqtt generic the qos:" << this->generic_qos << std::endl;
    this->generic_clean_session = toml::find<bool>(generic, "clean_session");
    std::cout << "[Bridge]mqtt generic the clean_session: "
              << (this->generic_clean_session ? "true" : "false") << std::endl;
    this->generic_client_id = toml::find<std::string>(generic, "client_id");
    std::cout << "[Bridge]mqtt generic the client_id: " << this->generic_client_id << std::endl;
    this->generic_ca_cert = toml::find<std::string>(generic, "ca_cert");
    std::cout << "[Bridge]mqtt generic the ca_cert: " << this->generic_ca_cert << std::endl;
    this->generic_tls_cert = toml::find<std::string>(generic, "tls_cert");
    std::cout << "[Bridge]mqtt generic the tls_cert: " << this->generic_tls_cert << std::endl;
    this->generic_tls_key = toml::find<std::string>(generic, "tls_key");
    std::cout << "[Bridge]mqtt generic the tls_key: " << this->generic_tls_key << std::endl;
}

void BridgeToml::parse_local_for_each(void)
{
    this->parse_toml_backend_udp();
    this->parse_toml_integration_generic();
}

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

static int lora_bridge_set_mqtt_topic(void)
{
    ifstream json_ifstream;
    ofstream json_ofstream;
    json     local_json;
    string   eui;
    try {
        json_ifstream.open(BRIDGE_TOPIC_CONF_DEFAULT);
        json_ifstream >> local_json;
        json_ifstream.close();
        eui = local_json["gateway_eui"];
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
    }
    char gateway_eui[MAX_GATEWAY_ID + 1] = { 0 };
    if (generate_gateway_id_by_mac(gateway_eui) < 0) {
        std::cerr << "Failed to get eth mac." << std::endl;
        return -1;
    }
    if (eui != string(gateway_eui)) {
        topic_sub_txpk = string("gateway/") + string(gateway_eui) + string("/event/") + string("tx");
    } else {
        std::cout << "Topic has been writen to file." << std::endl;
        topic_sub_txpk = local_json["topic_sub_txpk"];
    }
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
    tx_json["txpk"]["freq"] = tx_freq;
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

static int parse_bridge_toml_file(void)
{
    BridgeToml toml;
    try { // 读取lorabridge toml 配置参数
        toml.get_bridge_config_info();
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return -1;
    }
    return 0;
}

int main(int argc, char const *argv[])
{
    if (argc < 2) {
        std::cerr << "Format:" << argv[0] << " " << "{{feq}} e.g:" << argv[0] << " 923.123456" << std::endl;
        printf("US915 feq: min: 923.00 max: 928.00 \n");
        printf("EU868 feq: min: 863.00 max: 870.00 \n");
        printf("US915 feq: min: 500.00 max: 510.00 \n");
        return -1;
    } else {
        tx_freq = std::atof(argv[1]);
        if (tx_freq <= (double)0) {
            std::cerr << "请输入有效的浮点类型." << std::endl;
            std::cerr << "Format:" << argv[0] << " " << "{{feq}} e.g:" << argv[0] << " 923.123456" << std::endl;
            printf("US915 feq: min: 923.00 max: 928.00 \n");
            printf("EU868 feq: min: 863.00 max: 870.00 \n");
            printf("US915 feq: min: 500.00 max: 510.00 \n");
            return -1;
        }
    }
    if (parse_bridge_toml_file() < 0) {
        std::cerr << "Failed to parse bridge toml file." << std::endl;
        return -1;
    }
    if (lora_bridge_set_mqtt_topic() < 0) {
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
    // 设置用户名和密码
    if (!mqtt_username.empty() && !mqtt_password.empty()) {
        std::cout << "Set username and password..." << std::endl;
        mosquitto_username_pw_set(mosq, mqtt_username.c_str(), mqtt_password.c_str());
    }
    // 设置TLS选项
    if (!ca_file_path.empty() && !key_file_path.empty() && !cert_file_path.empty()) {
        std::cout << "Set TLS encryption...." << std::endl;
        string folder_path;
        auto pos = ca_file_path.find_last_not_of("/\\");
        if (pos != string::npos) {
            folder_path = ca_file_path.substr(0, pos);
        }
        if (folder_path.empty()) {
            std::cerr << "Either cafile or capath must not be empty" << std::endl;
            mosquitto_destroy(mosq);
            mosquitto_lib_cleanup();
            return -1;
        }
        printf("Cafile: %s, Cafile path: %s, Certfile:%s, Keyfile: %s\n",
                ca_file_path.c_str(), folder_path.c_str(), cert_file_path.c_str(), key_file_path.c_str());
        mosquitto_tls_set(mosq, ca_file_path.c_str(), folder_path.c_str(),
                                cert_file_path.c_str(), key_file_path.c_str(), NULL);
    }
    mosquitto_connect_callback_set(mosq, on_connect_publish);
    mosquitto_publish_callback_set(mosq, on_publish);
    ret = mosquitto_connect(mosq, mqtt_host.c_str(), mqtt_port, mqtt_keepalive);
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
        sleep(5);
    }
}