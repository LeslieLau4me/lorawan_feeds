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
using namespace std;
using json = nlohmann::json;

struct mosquitto  *mosq   = nullptr;
struct event_base *evbase = nullptr;

static int             mqtt_keepalive = 60;
static evutil_socket_t udp_socket;

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

static bool has_connected = false;

/* Topic for publish*/

static string topic_pub_rxpk;
static string topic_pub_downlink;
static string topic_pub_downlink_ack;
static string topic_pub_gateway_stat;

/* Topic for subscribe*/

static string topic_sub_txpk;

static json uplink_rx_json;
static json uplink_stat_json;
static json uplink_json;
static json downlink_json;

//上下行buffer 和packet forwarder 保持一致
uint8_t            buffer_up[TX_BUFF_SIZE] = { 0 };
uint8_t            buffer_down[1000]       = { 0 };
struct sockaddr_in client_addr;
socklen_t          client_len = sizeof(client_addr);

/*
存储订阅到的下行数据，即应用服务器发布的消息，节点内容如下：
{"txpk":{
"imme":true,
"freq":861.3,
"rfch":0,
"powe":12,
"modu":"FSK",
"datr":50000,
"fdev":3000,
"size":32,
"data":"H3P3N2i9qc4yt7rK7ldqoeCVJGBybzPY5h1Dd7P7p8v"
}}
*/
queue<string>   queue_downlink;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

static int  response_pkt_push_data(evutil_socket_t fd);
static int  response_pkt_pull_data(evutil_socket_t fd);
static int  recieve_pkt_tx_ack(evutil_socket_t fd);
typedef int (*udp_pkt_cb)(evutil_socket_t fd);

map<int, udp_pkt_cb> map_udp_pkt_cb = {
    { PKT_PUSH_DATA, response_pkt_push_data },
    { PKT_PULL_DATA, response_pkt_pull_data },
    { PKT_TX_ACK, recieve_pkt_tx_ack },
};

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

static void signal_cb(evutil_socket_t sig, short events, void *user_data)
{
    printf("INFO: sig:[%d], LoRa gateway bridge will exit...\n", sig);
    event_base_loopexit(evbase, NULL);
}

// Mosquitto连接回调函数
static void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
    printf("on_connect: %s\n", mosquitto_connack_string(rc));
    if (rc != 0) {
        std::cerr << "Failed to connect to MQTT broker." << std::endl;
        mosquitto_disconnect(mosq);
    } else {
        std::cout << "Connected to MQTT broker." << std::endl;
        if (topic_sub_txpk.empty() || mosquitto_subscribe(mosq, NULL, topic_sub_txpk.c_str(), mqtt_qos) < 0) {
            std::cerr << "Failed to subscribe tx topic." << std::endl;
        }
    }
}

void on_disconnect(struct mosquitto *mosq, void *userdata, int result)
{
    printf("WARN: MQTT broker had lost connection, LoRa gateway bridge will exit...\n");
}

static void
on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
    printf("Topic subscribed...\n");
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

// Mosquitto发布回调函数
static void on_publish(struct mosquitto *mosq, void *obj, int mid)
{
    std::cout << "Message published." << std::endl;
}

/*
### 3.2. PUSH_DATA packet ###
That packet type is used by the gateway mainly to forward the RF packets
received, and associated metadata, to the server.

 Bytes  | Function
:------:|---------------------------------------------------------------------
 0      | protocol version = 2
 1-2    | random token
 3      | PUSH_DATA identifier 0x00
 4-11   | Gateway unique identifier (MAC address)
 12-end | JSON object, starting with {, ending with }

 ### 3.3. PUSH_ACK packet ###

That packet type is used by the server to acknowledge immediately all the
PUSH_DATA packets received.

 Bytes  | Function
:------:|---------------------------------------------------------------------
 0      | protocol version = 2
 1-2    | same token as the PUSH_DATA packet to acknowledge
 3      | PUSH_ACK identifier 0x01
*/
static int response_pkt_push_data(evutil_socket_t fd)
{
    uint8_t ack[32] = { 0 };
    ack[0]          = buffer_up[0];
    ack[1]          = buffer_up[1];
    ack[2]          = buffer_up[2];
    ack[3]          = PKT_PUSH_ACK;
    sendto(fd, ack, sizeof(ack), 0, (struct sockaddr *)&client_addr, client_len);
    uplink_json.clear();
    uplink_rx_json.clear();
    uplink_stat_json.clear();
    /* clang-format off */
    try {
        uplink_json = json::parse(buffer_up + 12);
        if (!topic_pub_gateway_stat.empty()) {
            if (uplink_json.contains("stat")) {
                // uplink_stat_json         = uplink_json["stat"];
                uplink_stat_json["stat"] = uplink_json["stat"];
                string str_stat          = uplink_stat_json.dump();
                std::cout << "publish topic:" << topic_pub_gateway_stat << std::endl;
                mosquitto_publish(mosq, NULL, topic_pub_gateway_stat.c_str(), str_stat.length(), str_stat.c_str(), mqtt_qos, false);
            }
        }
        if (!topic_pub_rxpk.empty()) {
            if (uplink_json.contains("rxpk")) {
                // uplink_rx_json           = uplink_json["rxpk"];
                uplink_rx_json["rxpk"] = uplink_json["rxpk"];
                string str_rxpk        = uplink_rx_json.dump();
                // 将上行数据发布到MQTT主题
                mosquitto_publish(mosq, NULL, topic_pub_rxpk.c_str(), str_rxpk.length(), str_rxpk.c_str(), mqtt_qos, false);
                std::cout << "publish topic:" << topic_pub_rxpk << std::endl;
            }
        }
        std::cout << "uplink json:" << uplink_json.dump() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return -1;
    }
    return 0;
}

/*
### 5.5. TX_ACK packet ###

That packet type is used by the gateway to send a feedback to the server
to inform if a downlink request has been accepted or rejected by the gateway.
The datagram may optionnaly contain a JSON string to give more details on
acknoledge. If no JSON is present (empty string), this means than no error
occured.

 Bytes  | Function
:------:|---------------------------------------------------------------------
 0      | protocol version = 2
 1-2    | same token as the PULL_RESP packet to acknowledge
 3      | TX_ACK identifier 0x05
 4-11   | Gateway unique identifier (MAC address)
 12-end | [optional] JSON object, starting with {, ending with }, see section 6
*/
static int recieve_pkt_tx_ack(evutil_socket_t fd)
{
    /* clang-format on */
    (void)fd;
    try {
        downlink_json = json::parse(buffer_up + 12);
        if (!topic_pub_downlink_ack.empty()) {
            if (downlink_json.contains("txpk_ack")) {
                string str_txack = downlink_json.dump();
                /* clang-format off */
                mosquitto_publish(mosq, NULL, topic_pub_downlink_ack.c_str(), str_txack.length(), str_txack.c_str(), mqtt_qos, false);
                std::cout << "publish topic:" << topic_pub_downlink_ack << ":" << str_txack << std::endl;
            }
        }

    } catch (const std::exception &e) {
        std::cout << "Tx packet ok." << std::endl;
        return -1;
    }
    return 0;
}

/*
### 5.2. PULL_DATA packet ###
 Bytes  | Function
:------:|---------------------------------------------------------------------
 0      | protocol version = 2
 1-2    | random token
 3      | PULL_DATA identifier 0x02
 4-11   | Gateway unique identifier (MAC address)

### 5.3. PULL_ACK packet ###

That packet type is used by the server to confirm that the network route is
open and that the server can send PULL_RESP packets at any time.

 Bytes  | Function
:------:|---------------------------------------------------------------------
 0      | protocol version = 2
 1-2    | same token as the PULL_DATA packet to acknowledge
 3      | PULL_ACK identifier 0x04
 ### 5.4. PULL_RESP packet ###

That packet type is used by the server to send RF packets and associated
metadata that will have to be emitted by the gateway.

 Bytes  | Function
:------:|---------------------------------------------------------------------
 0      | protocol version = 2
 1-2    | random token
 3      | PULL_RESP identifier 0x03
 4-end  | JSON object, starting with {, ending with }, see section 6
*/
static int response_pkt_pull_data(evutil_socket_t fd)
{
    pthread_mutex_lock(&queue_mutex);
    // 无数据下发则发ack，有数据则发数据
    if (queue_downlink.empty()) {
        uint8_t ack[32] = { 0 };
        ack[0]          = buffer_up[0];
        ack[1]          = buffer_up[1];
        ack[2]          = buffer_up[2];
        ack[3]          = PKT_PULL_ACK;
        sendto(fd, ack, sizeof(ack), 0, (struct sockaddr *)&client_addr, client_len);
    } else {
        memset(buffer_down, 0, sizeof(buffer_down));
        string downlink_msg = queue_downlink.front();
        queue_downlink.pop();
        buffer_down[0] = buffer_up[0];
        buffer_down[1] = buffer_up[1];
        buffer_down[2] = buffer_up[2];
        buffer_down[3] = PKT_PULL_RESP;
        memcpy(buffer_down + 4, downlink_msg.c_str(), downlink_msg.length());
        /* clang-format off */
        sendto(fd, buffer_down, sizeof(buffer_down), 0, (struct sockaddr *)&client_addr, client_len);
        try {
            downlink_json = json::parse(downlink_msg);
            if (!topic_pub_downlink.empty()) {
                if (downlink_json.contains("txpk")) {
                    string str_txpk = downlink_json.dump();
                    mosquitto_publish(mosq, NULL, topic_pub_downlink.c_str(), str_txpk.length(), str_txpk.c_str(), mqtt_qos, false);
                    std::cout << "publish topic:" << topic_pub_downlink << ":" << str_txpk << std::endl;
                }
            }
            /* clang-format on */
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
            pthread_mutex_unlock(&queue_mutex);
            return -1;
        }
    }
    pthread_mutex_unlock(&queue_mutex);
    return 0;
}

static void read_cb(evutil_socket_t fd, short events, void *arg)
{
    memset(buffer_up, 0, sizeof(buffer_up));
    auto n =
        recvfrom(fd, buffer_up, sizeof(buffer_up), 0, (struct sockaddr *)&client_addr, &client_len);
    if (n <= 0 || static_cast<int>(buffer_up[0]) != PROTOCOL_VERSION) {
        return;
    }
    int mode = static_cast<int>(buffer_up[3]);
    if (map_udp_pkt_cb.count(mode)) {
        // 执行消息处理的回调
        int ret = map_udp_pkt_cb[mode](fd);
        if (ret < 0) {}
    }
}

// UDP面向无连接， TCP面向缓冲区，前者不能用bufferevent
static void bufferevent_read_cb(struct bufferevent *bev, void *ctx)
{
    struct evbuffer *input = bufferevent_get_input(bev);
    size_t           len   = evbuffer_get_length(input);
    if (len > 0) {
        std::vector<uint8_t> buffer(len);
        evbuffer_copyout(input, buffer.data(), len);
        // 处理接收到的LoRaWAN上行数据
        // 清空缓冲区
        evbuffer_drain(input, len);
    }
}

static void
on_message(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
    std::cout << "Received MQTT message on topic: " << message->topic << std::endl;
    std::string payload(static_cast<const char *>(message->payload), message->payloadlen);
    pthread_mutex_lock(&queue_mutex);
    queue_downlink.push(payload);
    pthread_mutex_unlock(&queue_mutex);
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

static int get_local_eth_mac(unsigned char *mac_address)
{
    struct ifreq ifr;
    int          sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to socket." << '\n';
        return -1;
    }
    strncpy(ifr.ifr_name, ETH_NAME_DEFAULT, IFNAMSIZ - 1);
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
        json setting_json;
        setting_json["gateway_eui"]    = gateway_eui;
        setting_json["topic_pub_rxpk"] = topic_pub_rxpk =
            string("gateway/") + string(gateway_eui) + string("/event/") + string("up");
        setting_json["topic_pub_downlink"] = topic_pub_downlink =
            string("gateway/") + string(gateway_eui) + string("/event/") + string("down");
        setting_json["topic_pub_downlink_ack"] = topic_pub_downlink_ack =
            string("gateway/") + string(gateway_eui) + string("/event/") + string("ack");
        setting_json["topic_pub_gateway_stat"] = topic_pub_gateway_stat =
            string("gateway/") + string(gateway_eui) + string("/event/") + string("stat");
        setting_json["topic_sub_txpk"] = topic_sub_txpk =
            string("gateway/") + string(gateway_eui) + string("/event/") + string("tx");
        json_ofstream.open(BRIDGE_TOPIC_CONF_DEFAULT);
        json_ofstream << setting_json.dump(4) << endl;
        json_ofstream.close();
    } else {
        std::cout << "Topic has been writen to file." << std::endl;
        topic_pub_rxpk         = local_json["topic_pub_rxpk"];
        topic_pub_downlink     = local_json["topic_pub_downlink"];
        topic_pub_downlink_ack = local_json["topic_pub_downlink_ack"];
        topic_pub_gateway_stat = local_json["topic_pub_gateway_stat"];
        topic_sub_txpk         = local_json["topic_sub_txpk"];
    }

    std::cout << "Uplink rx topic:" << topic_pub_rxpk << std::endl;
    std::cout << "Downlink tx topic:" << topic_pub_downlink << std::endl;
    std::cout << "Downlink tx ack topic:" << topic_pub_downlink_ack << std::endl;
    std::cout << "Gateway statistics topic:" << topic_pub_gateway_stat << std::endl;
    std::cout << "Tx topic receiving tx packet:" << topic_sub_txpk << std::endl;

    return 0;
}

void *mqtt_message_thread(void *arg)
{
    pthread_detach(pthread_self());
    int ret = mosquitto_connect(mosq, mqtt_host.c_str(), mqtt_port, mqtt_keepalive);
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error: %s, program exit....\n", mosquitto_strerror(ret));
        event_base_loopexit(evbase, NULL);
        return NULL;
    }
    has_connected = true;
    mosquitto_loop_forever(mosq, -1, 1);
    return NULL;
}

int main(void)
{
    if (parse_bridge_toml_file() < 0) {
        std::cerr << "Failed to parse bridge toml file." << std::endl;
        return -1;
    }

    if (lora_bridge_set_mqtt_topic() < 0) {
        std::cerr << "Failed to setup mqtt topic." << std::endl;
        return -1;
    }
    // 初始化Mosquitto库
    mosquitto_lib_init();

    // 创建Mosquitto客户端
    mosq = mosquitto_new(nullptr, mqtt_clean_session, nullptr);
    if (!mosq) {
        std::cerr << "Failed to create Mosquitto client." << std::endl;
        return -1;
    }

    // 设置连接回调函数
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_subscribe_callback_set(mosq, on_subscribe);
    // 设置发布回调函数
    mosquitto_publish_callback_set(mosq, on_publish);
    mosquitto_message_callback_set(mosq, on_message);

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

    // 创建事件处理器
    evbase = event_base_new();
    if (!evbase) {
        std::cerr << "Failed to create event base." << std::endl;
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return -1;
    }

    // 创建UDP套接字和事件
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == -1) {
        std::cerr << "Failed to create UDP socket." << std::endl;
        event_base_free(evbase);
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return -1;
    }

    struct event *udp_ev = event_new(evbase, udp_socket, EV_READ | EV_PERSIST, read_cb, NULL);
    if (!udp_ev || event_add(udp_ev, NULL) < 0) {
        std::cerr << "Failed to create udp event." << std::endl;
        event_base_free(evbase);
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return -1;
    }

    sockaddr_in serveraddr{};
    serveraddr.sin_family      = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port        = htons(LORAWAN_UDP_PORT);

    if (bind(udp_socket, reinterpret_cast<const sockaddr *>(&serveraddr), sizeof(serveraddr)) ==
        -1) {
        std::cerr << "Failed to bind socket." << std::endl;
        event_free(udp_ev);
        event_base_free(evbase);
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return -1;
    }

    struct event *signal_event = evsignal_new(evbase, SIGINT, signal_cb, NULL);
    if (!signal_event || event_add(signal_event, NULL) < 0) {
        std::cerr << "Could not create/add a signal event!" << std::endl;
        event_free(udp_ev);
        event_base_free(evbase);
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return -1;
    }

    pthread_t mqtt_tid;
    pthread_create(&mqtt_tid, NULL, mqtt_message_thread, NULL);

    event_base_dispatch(evbase);
    close(udp_socket);
    if (has_connected) {
        mosquitto_loop_stop(mosq, false);
    }
    event_base_free(evbase);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
