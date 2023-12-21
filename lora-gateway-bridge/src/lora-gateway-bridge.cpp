#include "lora-gateway-bridge.hpp"
#include "base64.hpp"

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
static string  tls_pass_phrase;
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

static char   gateway_eui[MAX_GATEWAY_ID + 1] = { 0 };
static string local_ip;
static Base64 base_64_obj;

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

static int response_pkt_push_data(evutil_socket_t fd);
static int response_pkt_pull_data(evutil_socket_t fd);
static int recieve_pkt_tx_ack(evutil_socket_t fd);
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
    string   generic_pass_phrase;

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
    tls_pass_phrase    = this->generic_pass_phrase;
}

void BridgeToml::parse_toml_backend_udp(void)
{
    const auto &backend = toml::find(toml_data, "backend");
    this->backend_type  = toml::find<std::string>(backend, "type");

    const auto &semtech_udp = toml::find(backend, "semtech_udp");
    string      udp_bind    = toml::find<std::string>(semtech_udp, "udp_bind");
    auto        idx         = udp_bind.find(":");
    char       *ip_port     = const_cast<char *>(udp_bind.c_str());
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
    auto       idx               = bind.find(":");
    char      *ip_port           = const_cast<char *>(bind.c_str());
    string     actual_ip;
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
    }
    this->generic_username      = toml::find<std::string>(generic, "username");
    this->generic_password      = toml::find<std::string>(generic, "password");
    this->generic_qos           = toml::find<std::uint32_t>(generic, "qos");
    this->generic_clean_session = toml::find<bool>(generic, "clean_session");
    this->generic_client_id     = toml::find<std::string>(generic, "client_id");
    this->generic_ca_cert       = toml::find<std::string>(generic, "ca_cert");
    this->generic_tls_cert      = toml::find<std::string>(generic, "tls_cert");
    this->generic_tls_key       = toml::find<std::string>(generic, "tls_key");
    this->generic_pass_phrase   = toml::find<std::string>(generic, "tls_pass_phrase");
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
        if (topic_sub_txpk.empty() ||
            mosquitto_subscribe(mosq, NULL, topic_sub_txpk.c_str(), mqtt_qos) < 0) {
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

map<string, uint8_t> map_dr = {
    { "SF7", 7 }, { "SF8", 8 }, { "SF9", 9 }, { "SF10", 10 }, { "SF11", 11 }, { "SF12", 12 },
};

map<string, uint16_t> map_bw = {
    { "BW125", 125 },
    { "BW250", 250 },
    { "BW125", 500 },
};

static int parse_uplink_datr(string datr, uint8_t &dr, uint16_t &bw)
{
    bool dr_match = false;
    bool bw_match = false;
    for (const auto &iter : map_dr) {
        if (datr.find(iter.first) != string::npos) {
            dr       = iter.second;
            dr_match = true;
            break;
        }
    }
    for (const auto &iter : map_bw) {
        if (datr.find(iter.first) != string::npos) {
            bw       = iter.second;
            bw_match = true;
            break;
        }
    }
    if (dr_match == false || bw_match == false) {
        std::cout << "WARN: wrong daterate or bw_match :" << datr << std::endl;
        return -1;
    }
    return 0;
}

/*
{
    "phyPayload": "AAEBAQEBAQEBAQEBAQEBAQGXFgzLPxI=",  // base64 encoded LoRaWAN frame
    "txInfo": {
        "frequency": 868300000,
        "modulation": "LORA",
        "loRaModulationInfo": {
            "bandwidth": 125,
            "spreadingFactor": 11,
            "codeRate": "4/5",
            "polarizationInversion": false
        }
    },
    "rxInfo": {
        "gatewayID": "cnb/AC4GLBg=",
        "time": "2018-07-26T15:15:58.599497Z",         // only set when the gateway has a GPS time source
        "timestamp": 58692860,                         // gateway internal timestamp (23 bit)
        "rssi": -55,
        "loRaSNR": 15,
        "channel": 2,
        "rfChain": 0,
        "board": 0,
        "antenna": 0,
        "fineTimestampType": "ENCRYPTED",
        "encryptedFineTimestamp": {
            "aesKeyIndex": 0,
            "encryptedNS": "d2YFe51PraE3EpnrZJV4aw=="  // encrypted nanosecond part of the time
        }
    }
}
*/
static void publish_chirpstack_format_uplink_json(json &json_up)
{
    string str_rxpk;
    float freq = 0.0;
    for (const auto &rxpk : uplink_json["rxpk"]) {
        json_up["phyPayload"] = rxpk["data"];
        if (rxpk["freq"].is_number_float()) {
            freq = rxpk["freq"];
            json_up["txInfo"]["frequency"] = freq * 1000000;
        } else {
            continue;
        }
        json_up["txInfo"]["modulation"] = rxpk["modu"];
        string datr                     = rxpk["datr"];
        if (!datr.empty()) {
            uint8_t  dr;
            uint16_t bw;
            if (parse_uplink_datr(datr, dr, bw) < 0) {
                continue;
            }
            json_up["txInfo"]["loRaModulationInfo"]["bandwidth"]       = bw;
            json_up["txInfo"]["loRaModulationInfo"]["spreadingFactor"] = dr;
            json_up["txInfo"]["loRaModulationInfo"]["codeRate"]        = rxpk["codr"];
            // push 为 false，pull 为true
            json_up["txInfo"]["loRaModulationInfo"]["polarizationInversion"] = false;
        }
        json_up["rxInfo"]["gatewayID"] = base_64_obj.encode(string(gateway_eui));
        if (rxpk.contains("time")) {
            json_up["rxInfo"]["time"] = rxpk["time"];
        }
        json_up["rxInfo"]["timestamp"] = rxpk["tmms"];
        json_up["rxInfo"]["rssi"]      = rxpk["rssi"];
        json_up["rxInfo"]["loRaSNR"]   = rxpk["lsnr"];
        json_up["rxInfo"]["rfChain"]   = rxpk["rfch"];
        json_up["rxInfo"]["board"]     = rxpk["brd"];

        if (rxpk.contains("brd")) {
            json_up["rxInfo"]["board"] = rxpk["brd"];
        } else {
            json_up["rxInfo"]["board"] = 0;
        }

        if (rxpk.contains("antenna")) {
            json_up["rxInfo"]["antenna"] = rxpk["rsig"];
        } else {
            json_up["rxInfo"]["antenna"] = 0;
        }
        str_rxpk.clear();
        str_rxpk = json_up.dump();
        /* clang-format off */
        std::cout << "publish topic:" << topic_pub_rxpk << std::endl;
        mosquitto_publish(mosq, NULL, topic_pub_rxpk.c_str(), str_rxpk.length(), str_rxpk.c_str(), mqtt_qos, false);
        /* clang-format on */
    }
}

static string get_iface_ip_address(void)
{
    struct ifaddrs *ifaddr, *ifa;
    char            ip[INET_ADDRSTRLEN];
    auto iface_name = "eth0.2";

    if (getifaddrs(&ifaddr) == -1) {
        perror("Failed to get interface addresses");
        return "";
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }
        struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
        inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN);
        if (strcmp(ifa->ifa_name, iface_name)) {
            printf("Interface: %s, IP: %s\n", ifa->ifa_name, ip);
            freeifaddrs(ifaddr);
            return string(ip);
        }
    }

    freeifaddrs(ifaddr);
    return "";
}

static void publish_chirpstack_format_stat_json(json &json_stat)
{
    string str_stat;
    json_stat["gatewayID"] = base_64_obj.encode(string(gateway_eui));
    if (local_ip.empty()) {
        local_ip        = get_iface_ip_address();
        json_stat["ip"] = local_ip;
    } else {
        json_stat["ip"] = local_ip;
    }

    json_stat["configVersion"]       = "1.2.3";
    json_stat["rxPacketsReceived"]   = json_stat["stat"]["rxnb"];
    json_stat["rxPacketsReceivedOK"] = json_stat["stat"]["rxok"];
    json_stat["txPacketsReceived"]   = json_stat["stat"]["dwnb"];
    json_stat["txPacketsEmitted"]    = json_stat["stat"]["txnb"];

    str_stat = json_stat.dump();
    std::cout << "publish topic:" << topic_pub_gateway_stat << std::endl;
    /* clang-format off */
    mosquitto_publish(mosq, NULL, topic_pub_gateway_stat.c_str(), str_stat.length(), str_stat.c_str(), mqtt_qos, false);
    /* clang-format on */
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
                publish_chirpstack_format_stat_json(uplink_stat_json);
            }
        }
        if (!topic_pub_rxpk.empty()) {
            if (uplink_json.contains("rxpk")) {
                // uplink_rx_json           = uplink_json["rxpk"];
                uplink_rx_json["rxpk"] = uplink_json["rxpk"];
                publish_chirpstack_format_uplink_json(uplink_rx_json);
            }
        }
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
    return 0;
}

int password_cb(char *buf, int size, int rwflag, void *userdata)
{
    std::cout << "===enter tls pass phrase===" << std::endl << tls_pass_phrase << std::endl;
    int len = tls_pass_phrase.length();
    if (len < size) {
        strcpy(buf, tls_pass_phrase.c_str());
        buf[size - 1] = '\0';
        return len;
    }
    return 0;
}

void *mqtt_message_thread(void *arg)
{
    pthread_detach(pthread_self());
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
        auto capath = "/etc/ssl";
        std::cout << "Set TLS encryption...." << std::endl;
        printf("Cafile: %s, Cafile path: %s, Certfile:%s, Keyfile: %s\n",
               ca_file_path.c_str(),
               capath,
               cert_file_path.c_str(),
               key_file_path.c_str());
        mosquitto_tls_set(mosq,
                          ca_file_path.c_str(),
                          capath,
                          cert_file_path.c_str(),
                          key_file_path.c_str(),
                          password_cb);
        mosquitto_tls_insecure_set(mosq, true);
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
        close(udp_socket);
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
        close(udp_socket);
        event_free(udp_ev);
        event_base_free(evbase);
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return -1;
    }

    struct event *signal_event = evsignal_new(evbase, SIGINT, signal_cb, NULL);
    if (!signal_event || event_add(signal_event, NULL) < 0) {
        std::cerr << "Could not create/add a signal event!" << std::endl;
        close(udp_socket);
        event_free(udp_ev);
        event_base_free(evbase);
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return -1;
    }
    int ret = mosquitto_connect(mosq, mqtt_host.c_str(), mqtt_port, mqtt_keepalive);
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error: %s, program exit....\n", mosquitto_strerror(ret));
        close(udp_socket);
        event_free(udp_ev);
        event_free(signal_event);
        event_base_free(evbase);
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return -1;
    }
    printf("Connected broker successfully, loop start....\n MQTT broker:%s:%d, QoS:%d, "
           "keepalive:%d \n",
           mqtt_host.c_str(),
           mqtt_port,
           (int)mqtt_qos,
           mqtt_keepalive);
    std::cout << "Uplink rx topic:" << topic_pub_rxpk << std::endl;
    std::cout << "Downlink tx topic:" << topic_pub_downlink << std::endl;
    std::cout << "Downlink tx ack topic:" << topic_pub_downlink_ack << std::endl;
    std::cout << "Gateway statistics topic:" << topic_pub_gateway_stat << std::endl;
    std::cout << "Tx topic receiving tx packet:" << topic_sub_txpk << std::endl;
    has_connected = true;

    pthread_t mqtt_tid;
    pthread_create(&mqtt_tid, NULL, mqtt_message_thread, NULL);

    event_base_dispatch(evbase);
    mosquitto_loop_stop(mosq, false);
    event_free(udp_ev);
    event_free(signal_event);
    event_base_free(evbase);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    close(udp_socket);
    return 0;
}
