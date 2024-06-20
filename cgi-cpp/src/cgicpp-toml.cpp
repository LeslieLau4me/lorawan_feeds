/**
 * @file
 * @brief  对LoRa Bridge 的toml类型配置文件进行读写操作
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 *
 * @details 识别具体的命令类型执行相应的回调、对配置文件进行修改或读取，返回json格式的数据内容和运行结果
 */

#include "cgicpp-toml.hpp"
#include "cgicpp-parser-base.hpp"
#include "cgicpp-lorawan-mode.hpp"
#include "easylogging++.h"
#include <unistd.h>

using namespace std;
using json = nlohmann::json;

class BridgeToml : public CgiParserBase
{
  private:
    toml::value toml_data;
    /* data */
    uint32_t log_level = 0;

    bool                     filter_enable = false;
    vector<string>           net_ids       = {};
    vector<array<string, 2>> join_euis     = {};

    string backend_type;
    // backend.semtech_udp
    string   udp_ip;
    uint32_t udp_port = 0;
    bool     skip_crc_check;
    bool     fake_rx_time;
    // backend.basic_station
    string   bs_ip;
    uint32_t bs_port = 0;
    string   bs_tls_cert;
    string   bs_tls_key;
    string   bs_ca_cert;
    uint32_t ping_interval = 0;
    uint32_t read_timeout  = 0;
    uint32_t write_timeout = 0;
    string   region;
    uint64_t frequency_min = 0;
    uint64_t frequency_max = 0;

    // integration
    string marshaler;
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
    uint16_t generic_qos = 0;
    bool     generic_clean_session;
    string   generic_client_id;
    string   generic_ca_cert;
    string   generic_tls_cert;
    string   generic_tls_key;
    string   generic_pass_phrase;

    // integration.mqtt.auth.gcp_cloud_iot_core
    string   gcp_server;
    string   gcp_device_id;
    string   gcp_project_id;
    string   gcp_cloud_region;
    string   gcp_registry_id;
    uint32_t gcp_jwt_expiration = 0;
    string   gcp_jwt_key_file;

    // integration.mqtt.auth.azure_iot_hub
    string   azure_device_connection_string;
    uint32_t azure_sas_token_expiration = 0;
    string   azure_device_id;
    string   azure_hostname;
    string   azure_tls_cert;
    string   azure_tls_key;

    bool show_more = false;
    // metrics
    bool metrics_endpoint_enabled;

    string   metrics_ip;
    uint32_t metrics_port = 0;
    // meta_data
    uint32_t meta_execution_interval     = 0;
    uint32_t meta_max_execution_duration = 0;

    ofstream outfile;
    ifstream infile;

    // json data used to upload
    json upload_json;
    json remote_json;

    string serialized_local_str;
    string serialized_remote_str;

    typedef void (BridgeToml::*toml_option_cb)(void);

    void run_parse_toml_cb(string fun_key);
    void parse_toml_general(void);
    void parse_toml_filters(void);
    void parse_toml_backend(void);
    void parse_toml_backend_udp(const toml::value &backend);
    void parse_toml_backend_bs(const toml::value &backend);
    void parse_toml_integration(void);
    void parse_toml_integration_generic(const toml::value &auth);
    void parse_toml_integration_azure(const toml::value &auth);
    void parse_toml_integration_gcp(const toml::value &auth);
    void parse_toml_metrics(void);
    void parse_toml_meta_data(void);
    void parse_toml_commands(void);

    void run_parse_json_cb(string fun_key);
    void parse_remote_json_string(void);
    void parse_remote_general(void);
    void parse_remote_filters(void);
    void parse_remote_backend(void);
    void parse_remote_integration(void);
    void parse_remote_metrics(void);
    void parse_remote_meta_data(void);
    void parse_remote_commands(void);

    void parse_toml_for_each(void);
    void parse_toml_for_one(void);

    void open_output_file(void);
    void assemble_data_part_1(void);
    void assemble_data_part_2(void);
    void assemble_data_part_3(void);
    void assemble_data_part_4(void);
    void assemble_data_part_5(void);
    void assemble_data_part_6(void);
    void assemble_data_part_7(void);
    void assemble_data_part_8(void);
    void output_formatted_toml_file(void);

    uint32_t parse_toml_string_to_sec(string t_str);
    string   parse_json_sec_to_string(uint32_t sec);
    // 单个选项
    string single_opt;
    void   run_gen_json_cb(string fun_key);
    void   generate_json_for_one_opt(void);
    void   generate_json_for_general(void);
    void   generate_json_for_filters(void);
    void   generate_json_for_backend(void);
    void   generate_json_for_integration(void);
    void   generate_json_for_metrics(void);
    void   generate_json_for_meta_data(void);
    void   generate_json_for_commands(void);
    void   generate_serialize_json(void);

    void set_generate_opt(string option);
    void update_local_toml_content(void);

    map<string, toml_option_cb> parse_toml_cb_map = {
        { GET_GENERAL, &BridgeToml::parse_toml_general },
        { GET_FILTERS, &BridgeToml::parse_toml_filters },
        { GET_BACKEND, &BridgeToml::parse_toml_backend },
        { GET_INTERGRATION, &BridgeToml::parse_toml_integration },
#ifdef USE_EXPAND
        { GET_METRICS, &BridgeToml::parse_toml_metrics },
        { GET_META_DATA, &BridgeToml::parse_toml_meta_data },
        { GET_COMMAND, &BridgeToml::parse_toml_commands },
#endif
    };
    map<string, toml_option_cb> parse_json_cb_map = {
        { SET_GENERAL, &BridgeToml::parse_remote_general },
        { SET_FILTERS, &BridgeToml::parse_remote_filters },
        { SET_BACKEND, &BridgeToml::parse_remote_backend },
        { SET_INTERGRATION, &BridgeToml::parse_remote_integration },
#ifdef USE_EXPAND
        { SET_METRICS, &BridgeToml::parse_remote_metrics },
        { SET_META_DATA, &BridgeToml::parse_remote_meta_data },
        { SET_COMMAND, &BridgeToml::parse_remote_commands },
#endif
    };
    map<string, toml_option_cb> gen_json_cb_map = {
        { GET_GENERAL, &BridgeToml::generate_json_for_general },
        { GET_FILTERS, &BridgeToml::generate_json_for_filters },
        { GET_BACKEND, &BridgeToml::generate_json_for_backend },
        { GET_INTERGRATION, &BridgeToml::generate_json_for_integration },
#ifdef USE_EXPAND
        { GET_METRICS, &BridgeToml::generate_json_for_metrics },
        { GET_META_DATA, &BridgeToml::generate_json_for_meta_data },
        { GET_COMMAND, &BridgeToml::generate_json_for_commands },
#endif
    };

  public:
    BridgeToml() : CgiParserBase("bridge toml file")
    {
        this->toml_data = toml::parse<toml::discard_comments>(BRIDGE_CONF_DEFAULT);
        // 如果存在未来得及删除的临时文件则先删掉
        if (access(TMP_FILE_NAME, F_OK) != -1) {
            remove(TMP_FILE_NAME);
        }
    }
    ~BridgeToml();

    void parse_local_for_each(void);
    void parse_local_for_one(string option);
    void set_local_for_each(void);
    void set_local_for_one(string option);
    void get_upload_json(string &input);
    void set_remote_string(string input);
};

void BridgeToml::set_remote_string(string input)
{
    this->serialized_remote_str = input;
}

void BridgeToml::get_upload_json(string &input)
{
    input = this->serialized_local_str;
}

void BridgeToml::parse_remote_json_string(void)
{
    this->remote_json = json::parse(this->serialized_remote_str);
}

void BridgeToml::set_generate_opt(string option)
{
    this->single_opt = option;
}

void BridgeToml::generate_json_for_one_opt(void)
{
    this->run_gen_json_cb(this->single_opt);
}

void BridgeToml::parse_toml_for_each(void)
{
    try {
        for (auto iter : this->parse_toml_cb_map) { this->run_parse_toml_cb(iter.first); }
    } catch (const std::exception &e) {
        (void)e;
        return;
    }
}

void BridgeToml::set_local_for_one(string option)
{
    this->parse_remote_json_string();
    // 保存原先的值
    this->parse_toml_for_each();
    // 从服务器下发的json中获取新值
    this->run_parse_json_cb(option);
    this->update_local_toml_content();
}

void BridgeToml::set_local_for_each(void)
{
    // 记录原文件的值
    this->parse_toml_for_each();
    this->parse_remote_json_string();
    this->parse_remote_general();
    this->parse_remote_filters();
    this->parse_remote_backend();
    this->parse_remote_integration();
#ifdef USE_EXPAND
    this->parse_remote_metrics();
    this->parse_remote_meta_data();
    this->parse_remote_commands();
#endif
    this->update_local_toml_content();
}

void BridgeToml::parse_toml_for_one(void)
{
    this->run_parse_toml_cb(this->single_opt);
}

void BridgeToml::parse_local_for_each(void)
{
    this->parse_toml_for_each();
    this->generate_json_for_general();
    this->generate_json_for_filters();
    this->generate_json_for_backend();
    this->generate_json_for_integration();
#ifdef USE_EXPAND
    this->generate_json_for_metrics();
    this->generate_json_for_meta_data();
    this->generate_json_for_commands();
#endif
    this->generate_serialize_json();
}

void BridgeToml::parse_local_for_one(string option)
{
    this->set_generate_opt(option);
    this->parse_toml_for_one();
    this->generate_json_for_one_opt();
    this->generate_serialize_json();
}

void BridgeToml::generate_serialize_json(void)
{
    // 缩进为4
    this->serialized_local_str = this->upload_json.dump(4);
}

void BridgeToml::generate_json_for_general(void)
{
    this->upload_json["log_level"] = this->log_level;
}

void BridgeToml::generate_json_for_filters(void)
{
    if (!(this->net_ids.empty())) {
        this->upload_json["net_ids"] = this->net_ids;
    }
    if (!(this->join_euis.empty())) {
        this->upload_json["join_euis"] = this->join_euis;
    }
    this->upload_json["filter_enable"] = this->filter_enable;
}

void BridgeToml::generate_json_for_backend(void)
{
    if (this->backend_type == string(SEMTECH_UDP)) {
        this->upload_json["backend_type"]                = SEMTECH_UDP;
        this->upload_json[SEMTECH_UDP]["udp_ip"]         = this->udp_ip;
        this->upload_json[SEMTECH_UDP]["udp_port"]       = this->udp_port;
        this->upload_json[SEMTECH_UDP]["skip_crc_check"] = this->skip_crc_check;
        this->upload_json[SEMTECH_UDP]["fake_rx_time"]   = this->fake_rx_time;
    } else if (this->backend_type == string(BASIC_STATION)) {
        this->upload_json["backend_type"]                 = BASIC_STATION;
        this->upload_json[BASIC_STATION]["bind_ip"]       = this->bs_ip;
        this->upload_json[BASIC_STATION]["bind_port"]     = this->bs_port;
        this->upload_json[BASIC_STATION]["tls_cert"]      = this->bs_tls_cert;
        this->upload_json[BASIC_STATION]["tls_key"]       = this->bs_tls_key;
        this->upload_json[BASIC_STATION]["ca_cert"]       = this->bs_ca_cert;
        this->upload_json[BASIC_STATION]["ping_interval"] = this->ping_interval;
        this->upload_json[BASIC_STATION]["write_timeout"] = this->write_timeout;
        this->upload_json[BASIC_STATION]["region"]        = this->region;
        this->upload_json[BASIC_STATION]["read_timeout"]  = this->read_timeout;
        this->upload_json[BASIC_STATION]["frequency_min"] = this->frequency_min;
        this->upload_json[BASIC_STATION]["frequency_max"] = this->frequency_max;
    }
}

void BridgeToml::generate_json_for_integration(void)
{
    this->upload_json["marshaler"]                    = this->marshaler;
    this->upload_json[MQTT]["event_topic_template"]   = this->event_topic_template;
    this->upload_json[MQTT]["command_topic_template"] = this->command_topic_template;
    this->upload_json[MQTT]["max_reconnect_interval"] = this->max_reconnect_interval;
    if (this->mqtt_auth_type == GENERIC) {
        this->upload_json["mqtt_auth_type"]           = GENERIC;
        this->upload_json[GENERIC]["ip"]              = this->generic_ip;
        this->upload_json[GENERIC]["port"]            = this->generic_port;
        this->upload_json[GENERIC]["username"]        = this->generic_username;
        this->upload_json[GENERIC]["password"]        = this->generic_password;
        this->upload_json[GENERIC]["qos"]             = this->generic_qos;
        this->upload_json[GENERIC]["clean_session"]   = this->generic_clean_session;
        this->upload_json[GENERIC]["client_id"]       = this->generic_client_id;
        this->upload_json[GENERIC]["ca_cert"]         = this->generic_ca_cert;
        this->upload_json[GENERIC]["tls_cert"]        = this->generic_tls_cert;
        this->upload_json[GENERIC]["tls_key"]         = this->generic_tls_key;
        this->upload_json[GENERIC]["tls_pass_phrase"] = this->generic_pass_phrase;
    } else if (this->mqtt_auth_type == GCP_CLOUND) {
        this->upload_json["mqtt_auth_type"]             = GCP_CLOUND;
        this->upload_json[GCP_CLOUND]["gcp_server"]     = this->gcp_server;
        this->upload_json[GCP_CLOUND]["device_id"]      = this->gcp_device_id;
        this->upload_json[GCP_CLOUND]["project_id"]     = this->gcp_project_id;
        this->upload_json[GCP_CLOUND]["cloud_region"]   = this->gcp_cloud_region;
        this->upload_json[GCP_CLOUND]["registry_id"]    = this->gcp_registry_id;
        this->upload_json[GCP_CLOUND]["jwt_expiration"] = this->gcp_jwt_expiration;
        this->upload_json[GCP_CLOUND]["jwt_key_file"]   = this->gcp_jwt_key_file;
    } else if (this->mqtt_auth_type == AZURE_IOT) {
        this->upload_json["mqtt_auth_type"] = AZURE_IOT;
        this->upload_json[AZURE_IOT]["device_connection_string"] =
            this->azure_device_connection_string;
        this->upload_json[AZURE_IOT]["sas_token_expiration"] = this->azure_sas_token_expiration;
        this->upload_json[AZURE_IOT]["device_id"]            = this->azure_device_id;
        this->upload_json[AZURE_IOT]["hostname"]             = this->azure_hostname;
        this->upload_json[AZURE_IOT]["tls_cert"]             = this->azure_tls_cert;
        this->upload_json[AZURE_IOT]["tls_key"]              = this->azure_tls_key;
    }
}

void BridgeToml::generate_json_for_metrics(void)
{
    // 默认不上报
    if (this->show_more) {
        this->upload_json["prometheus"]["endpoint_enabled"] = this->metrics_endpoint_enabled;
        this->upload_json["prometheus"]["ip"]               = this->metrics_ip;
        this->upload_json["prometheus"]["port"]             = this->metrics_port;
    }
}

void BridgeToml::generate_json_for_meta_data(void)
{
    if (this->show_more) {
        this->upload_json["static"]["serial_number"]           = "";
        this->upload_json["dynamic"]["max_execution_duration"] = this->meta_max_execution_duration;
        this->upload_json["dynamic"]["execution_interval"]     = this->meta_execution_interval;
        this->upload_json["dynamic"]["commands"]               = "";
    }
}

void BridgeToml::generate_json_for_commands(void)
{
    if (this->show_more) {
        this->upload_json["commands"]["reboot"] = "";
    }
}

BridgeToml::~BridgeToml() {}

void BridgeToml::run_gen_json_cb(string fun_key)
{
    if (this->gen_json_cb_map.count(fun_key)) {
        return (this->*gen_json_cb_map[fun_key])();
    }
}

void BridgeToml::run_parse_json_cb(string fun_key)
{
    if (this->parse_json_cb_map.count(fun_key)) {
        return (this->*parse_json_cb_map[fun_key])();
    }
}

void BridgeToml::run_parse_toml_cb(string fun_key)
{
    if (this->parse_toml_cb_map.count(fun_key)) {
        return (this->*parse_toml_cb_map[fun_key])();
    }
}

// reserve
void BridgeToml::parse_toml_commands(void) {}
void BridgeToml::parse_remote_commands(void) {}

string BridgeToml ::parse_json_sec_to_string(uint32_t sec)
{
    auto   hour   = sec / 3600;
    auto   min    = (sec % 3600) / 60;
    auto   second = (sec % 3600) % 60;
    string output;
    bool   has_hour = false;
    if (hour > 0) {
        output   = to_string(hour) + "h";
        has_hour = true;
    }
    if (min > 0 || has_hour == true) {
        output += to_string(min) + "m";
    }

    output += to_string(second) + "s";

    return output;
}

/*转换成s*/
uint32_t BridgeToml::parse_toml_string_to_sec(string t_str)
{
    uint32_t seconds = 0;
    char    *sec;
    char    *min;
    string   tmp_min, tmp_sec;
    tmp_min = tmp_sec = t_str;

    bool has_min  = false;
    bool has_hour = false;
    auto idx      = t_str.find('h');
    if (idx != string::npos) {
        char *interval = const_cast<char *>(t_str.c_str());
        char *hour     = strtok(interval, "h");
        seconds += atoi(hour) * 60 * 60;
        has_hour = true;
    }
    idx = t_str.find('m');
    if (idx != string::npos) {
        if (has_hour) {
            min = strtok(NULL, "m");
        } else {
            char *interval = const_cast<char *>(tmp_min.c_str());
            min            = strtok(interval, "m");
        }
        has_min = true;
        seconds += 60 * atoi(min);
    }

    idx = t_str.find('s');
    if (idx != string::npos) {
        if (has_min || has_hour) {
            sec = strtok(NULL, "s");
        } else {
            char *interval = const_cast<char *>(tmp_sec.c_str());
            sec            = strtok(interval, "s");
        }
        seconds += atoi(sec);
    }
    LOG(INFO) << "Total seconds is " << seconds;
    return seconds;
}

void BridgeToml::parse_toml_general(void)
{
    const auto &general = toml::find(this->toml_data, "general");
    this->log_level     = toml::find<std::uint32_t>(general, "log_level");
    LOG(INFO) << "[Bridge]the log_level is " << log_level;
}

void BridgeToml::parse_remote_general(void)
{
    // if (this->remote_json.contains("")) 不做判断
    this->log_level = this->remote_json[BODY]["log_level"];
}

void BridgeToml::parse_toml_filters(void)
{
    const auto &filters = toml::find(this->toml_data, "filters");
    try {
        this->net_ids       = toml::find<std::vector<string>>(filters, "net_ids");
        this->join_euis     = toml::find<vector<array<string, 2>>>(filters, "join_euis");
        this->filter_enable = true;
    } catch (const std::exception &e) {
        (void)e;
        this->filter_enable = false;
    }
}

void BridgeToml::parse_remote_filters(void)
{
    this->remote_json[BODY]["filter_enable"].get_to(this->filter_enable);
    if (this->filter_enable == true) {
        this->remote_json[BODY]["net_ids"].get_to(this->net_ids);
        this->remote_json[BODY]["join_euis"].get_to(this->join_euis);
    }
}

void BridgeToml::parse_toml_backend_udp(const toml::value &backend)
{
    const auto &semtech_udp = toml::find(backend, SEMTECH_UDP);
    string      udp_bind    = toml::find<std::string>(semtech_udp, "udp_bind");
    LOG(INFO) << "[Bridge] what udp bind is " << udp_bind;
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

void BridgeToml::parse_toml_backend_bs(const toml::value &backend)
{
    const auto &basic_station = toml::find(backend, BASIC_STATION);
    string      bind          = toml::find<std::string>(basic_station, "bind");
    LOG(INFO) << "[Bridge] Basicstation bind " << bind;
    auto  idx     = bind.find(":");
    char *ip_port = const_cast<char *>(bind.c_str());
    if (ip_port != NULL && idx > 0) {
        char *ip      = strtok(ip_port, ":");
        this->bs_ip   = string(ip);
        char *port    = strtok(NULL, ":");
        this->bs_port = atoi(port);
    }
    this->bs_tls_cert = toml::find<std::string>(basic_station, "tls_cert");
    LOG(INFO) << "[Bridge]Basicstation tls_cert is " << this->bs_tls_cert;
    this->bs_tls_key = toml::find<std::string>(basic_station, "tls_key");
    LOG(INFO) << "[Bridge]Basicstation tls_key is " << this->bs_tls_key;
    this->bs_ca_cert = toml::find<std::string>(basic_station, "ca_cert");
    LOG(INFO) << "[Bridge]Basicstation ca_cert is " << this->bs_ca_cert;
    string ping_interval = toml::find<std::string>(basic_station, "ping_interval");
    TOML_COUT << "[Bridge]Basicstation ping_interval is " << ping_interval << std::endl;
    this->ping_interval = this->parse_toml_string_to_sec(ping_interval);

    string read_timeout = toml::find<std::string>(basic_station, "read_timeout");
    TOML_COUT << "[Bridge]Basicstationthe read_timeout is " << read_timeout << std::endl;
    this->read_timeout = this->parse_toml_string_to_sec(read_timeout);

    string write_timeout = toml::find<std::string>(basic_station, "write_timeout");
    TOML_COUT << "[Bridge]Basicstationthe write_timeout is " << write_timeout << std::endl;
    this->write_timeout = this->parse_toml_string_to_sec(write_timeout);

    this->region = toml::find<std::string>(basic_station, "region");
    LOG(INFO) << "[Bridge]Basicstationthe region is " << this->region;
    this->frequency_min = toml::find<std::uint32_t>(basic_station, "frequency_min");
    LOG(INFO) << "[Bridge]Basicstationthe frequency_min is " << this->frequency_min;
    this->frequency_max = toml::find<std::uint32_t>(basic_station, "frequency_max");
    LOG(INFO) << "[Bridge]Basicstationthe frequency_max is " << this->frequency_max;
}

void BridgeToml::parse_toml_backend(void)
{
    const auto &backend = toml::find(toml_data, "backend");
    this->backend_type  = toml::find<std::string>(backend, "type");
    // 全部解析出来，便于模式切换
    this->parse_toml_backend_udp(backend);
    this->parse_toml_backend_bs(backend);
}

void BridgeToml::parse_remote_backend(void)
{
    this->remote_json[BODY]["backend_type"].get_to(this->backend_type);
    if (this->backend_type == SEMTECH_UDP) {
        this->remote_json[BODY][SEMTECH_UDP]["udp_ip"].get_to(this->udp_ip);
        this->remote_json[BODY][SEMTECH_UDP]["udp_port"].get_to(this->udp_port);
        this->remote_json[BODY][SEMTECH_UDP]["skip_crc_check"].get_to(this->skip_crc_check);
        this->remote_json[BODY][SEMTECH_UDP]["fake_rx_time"].get_to(this->fake_rx_time);
    } else if (this->backend_type == BASIC_STATION) {
        this->remote_json[BODY][BASIC_STATION]["bind_ip"].get_to(this->bs_ip);
        this->remote_json[BODY][BASIC_STATION]["bind_port"].get_to(this->bs_port);
        this->remote_json[BODY][BASIC_STATION]["ca_cert"].get_to(this->bs_ca_cert);
        this->remote_json[BODY][BASIC_STATION]["frequency_max"].get_to(this->frequency_max);
        this->remote_json[BODY][BASIC_STATION]["frequency_min"].get_to(this->frequency_min);
        this->remote_json[BODY][BASIC_STATION]["ping_interval"].get_to(this->ping_interval);
        this->remote_json[BODY][BASIC_STATION]["read_timeout"].get_to(this->read_timeout);
        this->remote_json[BODY][BASIC_STATION]["region"].get_to(this->region);
        this->remote_json[BODY][BASIC_STATION]["tls_cert"].get_to(this->bs_tls_cert);
        this->remote_json[BODY][BASIC_STATION]["tls_key"].get_to(this->bs_tls_key);
        this->remote_json[BODY][BASIC_STATION]["write_timeout"].get_to(this->write_timeout);
    }
}

void BridgeToml::parse_toml_integration_generic(const toml::value &auth)
{
    const auto generic = toml::find(auth, GENERIC);
    string     bind    = toml::find<std::string>(generic, "server");
    LOG(INFO) << "[Bridge]mqtt generic bind is " << bind;
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
        LOG(INFO) << "[Bridge]mqtt generic the port is " << port;
    }
    LOG(INFO) << "[Bridge]mqtt generic the server is " << this->generic_ip;
    this->generic_username = toml::find<std::string>(generic, "username");
    LOG(INFO) << "[Bridge]mqtt generic the username is " << this->generic_username;
    this->generic_password = toml::find<std::string>(generic, "password");
    LOG(INFO) << "[Bridge]mqtt generic the password is " << this->generic_password;
    this->generic_qos = toml::find<std::uint32_t>(generic, "qos");
    LOG(INFO) << "[Bridge]mqtt generic the qos is " << this->generic_qos;
    this->generic_clean_session = toml::find<bool>(generic, "clean_session");
    LOG(INFO) << "[Bridge]mqtt generic the clean_session is "
              << (this->generic_clean_session ? "true" : "false");
    this->generic_client_id = toml::find<std::string>(generic, "client_id");
    LOG(INFO) << "[Bridge]mqtt generic the client_id is " << this->generic_client_id;
    this->generic_ca_cert = toml::find<std::string>(generic, "ca_cert");
    LOG(INFO) << "[Bridge]mqtt generic the ca_cert is " << this->generic_ca_cert;
    this->generic_tls_cert = toml::find<std::string>(generic, "tls_cert");
    LOG(INFO) << "[Bridge]mqtt generic the tls_cert is " << this->generic_tls_cert;
    this->generic_tls_key = toml::find<std::string>(generic, "tls_key");
    LOG(INFO) << "[Bridge]mqtt generic the tls_key is " << this->generic_tls_key;
    this->generic_pass_phrase = toml::find<std::string>(generic, "tls_pass_phrase");
    LOG(INFO) << "[Bridge]mqtt generic the pass_phrase is " << this->generic_pass_phrase;
}

void BridgeToml::parse_toml_integration_gcp(const toml::value &auth)
{
    LOG(INFO) << "[Bridge] gcp_cloud_iot_core type";
    const auto gcp   = toml::find(auth, GCP_CLOUND);
    this->gcp_server = toml::find<std::string>(gcp, "server");
    LOG(INFO) << "[Bridge]gcp_cloud_iot_core server is " << this->gcp_server;
    this->gcp_device_id = toml::find<std::string>(gcp, "device_id");
    LOG(INFO) << "[Bridge]gcp_cloud_iot_core device_id is " << this->gcp_device_id;
    this->gcp_project_id = toml::find<std::string>(gcp, "project_id");
    LOG(INFO) << "[Bridge]gcp_cloud_iot_core password is " << this->gcp_project_id;
    this->gcp_cloud_region = toml::find<std::string>(gcp, "cloud_region");
    LOG(INFO) << "[Bridge]gcp_cloud_iot_core cloud_region is " << this->gcp_cloud_region;
    this->gcp_registry_id = toml::find<std::string>(gcp, "registry_id");
    LOG(INFO) << "[Bridge]gcp_cloud_iot_core registry_id is " << this->gcp_registry_id;
    string jwt_expiration = toml::find<std::string>(gcp, "jwt_expiration");
    LOG(INFO) << "[Bridge]gcp_cloud_iot_core jwt_expiration is " << jwt_expiration;
    this->gcp_jwt_expiration = parse_toml_string_to_sec(jwt_expiration);
    this->gcp_jwt_key_file   = toml::find<std::string>(gcp, "jwt_key_file");
    LOG(INFO) << "[Bridge]gcp_cloud_iot_core jwt_key_file is " << this->gcp_jwt_key_file;
}

void BridgeToml::parse_toml_integration_azure(const toml::value &auth)
{
    LOG(INFO) << "[Bridge]azure_iot_hub type";
    const auto azure = toml::find(auth, AZURE_IOT);
    this->azure_device_connection_string =
        toml::find<std::string>(azure, "device_connection_string");
    LOG(INFO) << "[Bridge]azure_iot_hub device_connection_string is "
              << this->azure_device_connection_string;
    string sas_token_expiration      = toml::find<std::string>(azure, "sas_token_expiration");
    this->azure_sas_token_expiration = parse_toml_string_to_sec(sas_token_expiration);
    LOG(INFO) << "[Bridge]azure_iot_hub sas_token_expiration is "
              << this->azure_sas_token_expiration;
    this->azure_device_id = toml::find<std::string>(azure, "device_id");
    LOG(INFO) << "[Bridge]azure_iot_hub device_id is " << this->azure_device_id;
    this->azure_hostname = toml::find<std::string>(azure, "hostname");
    LOG(INFO) << "[Bridge]azure_iot_hub hostname is " << this->azure_hostname;
    this->azure_tls_cert = toml::find<std::string>(azure, "tls_cert");
    LOG(INFO) << "[Bridge]azure_iot_hub registry_id is " << this->azure_tls_cert;
    this->azure_tls_key = toml::find<std::string>(azure, "tls_key");
    LOG(INFO) << "[Bridge]azure_iot_hub tls_key is " << this->azure_tls_key;
}

void BridgeToml::parse_toml_integration(void)
{
    // 定位到integration.mqtt
    const auto &integration = toml::find(this->toml_data, "integration");
    this->marshaler         = toml::find<std::string>(integration, "marshaler");
    LOG(INFO) << "[Bridge]mqtt marshaler is " << this->marshaler;
    const auto &mqtt           = toml::find(integration, "mqtt");
    this->event_topic_template = toml::find<std::string>(mqtt, "event_topic_template");
    LOG(INFO) << "[Bridge]mqtt event_topic_template is " << this->event_topic_template;
    this->command_topic_template = toml::find<std::string>(mqtt, "command_topic_template");
    LOG(INFO) << "[Bridge]mqtt command_topic_template is " << this->command_topic_template;
    string max_reconnect_interval = toml::find<std::string>(mqtt, "max_reconnect_interval");
    LOG(INFO) << "[Bridge]mqtt max_reconnect_interval is " << max_reconnect_interval;
    this->max_reconnect_interval = this->parse_toml_string_to_sec(max_reconnect_interval);

    const auto &auth     = toml::find(mqtt, "auth");
    this->mqtt_auth_type = toml::find<std::string>(auth, "type");
    LOG(INFO) << "[Bridge]mqtt auth_type is " << this->mqtt_auth_type;
    // 全部解析出来，便于模式切换
    this->parse_toml_integration_generic(auth);
    this->parse_toml_integration_azure(auth);
    this->parse_toml_integration_gcp(auth);
}

void BridgeToml::parse_remote_integration(void)
{
    this->remote_json[BODY][MQTT]["command_topic_template"].get_to(this->command_topic_template);
    this->remote_json[BODY][MQTT]["event_topic_template"].get_to(this->event_topic_template);
    this->remote_json[BODY][MQTT]["max_reconnect_interval"].get_to(this->max_reconnect_interval);
    this->remote_json[BODY]["marshaler"].get_to(this->marshaler);
    this->remote_json[BODY]["mqtt_auth_type"].get_to(this->mqtt_auth_type);
    if (this->mqtt_auth_type == GENERIC) {
        this->remote_json[BODY][GENERIC]["clean_session"].get_to(this->generic_clean_session);
        this->remote_json[BODY][GENERIC]["client_id"].get_to(this->generic_client_id);
        this->remote_json[BODY][GENERIC]["ip"].get_to(this->generic_ip);
        this->remote_json[BODY][GENERIC]["password"].get_to(this->generic_password);
        this->remote_json[BODY][GENERIC]["port"].get_to(this->generic_port);
        this->remote_json[BODY][GENERIC]["qos"].get_to(this->generic_qos);
        this->remote_json[BODY][GENERIC]["ca_cert"].get_to(this->generic_ca_cert);
        if (!this->generic_ca_cert.empty()) {
            remove_files_from_dir_except_input(string(BRIDGE_CAFILE_PATH), this->generic_ca_cert);
        }
        this->remote_json[BODY][GENERIC]["tls_cert"].get_to(this->generic_tls_cert);
        if (!this->generic_tls_cert.empty()) {
            remove_files_from_dir_except_input(string(BRIDGE_CERTFILE_PATH), this->generic_tls_cert);
        }
        this->remote_json[BODY][GENERIC]["tls_key"].get_to(this->generic_tls_key);
        if (!this->generic_tls_key.empty()) {
            remove_files_from_dir_except_input(string(BRIDGE_KEYFILE_PATH), this->generic_tls_key);
        }
        this->remote_json[BODY][GENERIC]["username"].get_to(this->generic_username);
        this->remote_json[BODY][GENERIC]["tls_pass_phrase"].get_to(this->generic_pass_phrase);
    } else if (this->mqtt_auth_type == GCP_CLOUND) {
        this->remote_json[BODY][GCP_CLOUND]["cloud_region"].get_to(this->gcp_cloud_region);
        this->remote_json[BODY][GCP_CLOUND]["device_id"].get_to(this->gcp_device_id);
        this->remote_json[BODY][GCP_CLOUND]["gcp_server"].get_to(this->gcp_server);
        this->remote_json[BODY][GCP_CLOUND]["jwt_expiration"].get_to(this->gcp_jwt_expiration);
        this->remote_json[BODY][GCP_CLOUND]["jwt_key_file"].get_to(this->gcp_jwt_key_file);
        this->remote_json[BODY][GCP_CLOUND]["project_id"].get_to(this->gcp_project_id);
        this->remote_json[BODY][GCP_CLOUND]["registry_id"].get_to(this->gcp_registry_id);
    } else if (this->mqtt_auth_type == AZURE_IOT) {
        this->remote_json[BODY][AZURE_IOT]["device_connection_string"].get_to(
            this->azure_device_connection_string);
        this->remote_json[BODY][AZURE_IOT]["device_id"].get_to(this->azure_device_id);
        this->remote_json[BODY][AZURE_IOT]["hostname"].get_to(this->azure_hostname);
        this->remote_json[BODY][AZURE_IOT]["sas_token_expiration"].get_to(
            this->azure_sas_token_expiration);
        this->remote_json[BODY][AZURE_IOT]["tls_cert"].get_to(this->azure_tls_cert);
        this->remote_json[BODY][AZURE_IOT]["tls_key"].get_to(this->azure_tls_key);
    }
}

void BridgeToml::parse_toml_metrics(void)
{
    // metrics
    const auto &metrics            = toml::find(this->toml_data, "metrics");
    const auto &prometheus         = toml::find(metrics, "prometheus");
    this->metrics_endpoint_enabled = toml::find<bool>(prometheus, "endpoint_enabled");
    LOG(INFO) << "[Bridge]metrics auth_type is "
              << (this->metrics_endpoint_enabled ? "true" : "false");

    string metrics_bind = toml::find<std::string>(prometheus, "bind");
    LOG(INFO) << "[Bridge]metrics  metrics_bind is " << metrics_bind;
    LOG(INFO) << "[Bridge]metrics  udp_bind is " << metrics_bind;
    if (!metrics_bind.empty()) {
        char *ip_port = const_cast<char *>(metrics_bind.c_str());
        if (ip_port != NULL) {
            char *ip         = strtok(ip_port, ":");
            this->metrics_ip = string(ip);
            LOG(INFO) << "[Bridge]metrics  ip is " << ip;
            char *port         = strtok(NULL, ":");
            this->metrics_port = atoi(port);
            LOG(INFO) << "[Bridge]metrics  port is " << port;
        }
    }
}

void BridgeToml::parse_remote_metrics(void) {}

void BridgeToml::parse_toml_meta_data(void)
{
    const auto &meta_data = toml::find(this->toml_data, "meta_data");
    const auto &m_static  = toml::find(meta_data, "static");
    (void)m_static;

    const auto &dynamic            = toml::find(meta_data, "dynamic");
    string      execution_interval = toml::find<std::string>(dynamic, "execution_interval");
    this->meta_execution_interval  = this->parse_toml_string_to_sec(execution_interval);
    LOG(INFO) << "[Bridge]meta_data the execution_interval is " << execution_interval;

    string max_execution_duration     = toml::find<std::string>(dynamic, "max_execution_duration");
    this->meta_max_execution_duration = this->parse_toml_string_to_sec(execution_interval);
    LOG(INFO) << "[Bridge]meta_data max_execution_duration is " << max_execution_duration;
}

void BridgeToml::parse_remote_meta_data(void) {}

void BridgeToml::open_output_file(void)
{
    this->outfile.open(TMP_FILE_NAME);
}

void BridgeToml::assemble_data_part_1(void)
{
    const toml::value data_part_1{
        { "general", { { "log_level", this->log_level } } },

    };
    this->outfile << data_part_1 << std::endl;
}

void BridgeToml::assemble_data_part_2(void)
{
    if (this->filter_enable == false) {
        this->net_ids.clear();
        this->join_euis.clear();
    }
    const toml::value data_part_2{
        { "filters", { { "net_ids", this->net_ids }, { "join_euis", this->join_euis } } },
    };
    this->outfile << data_part_2 << std::endl;
}

void BridgeToml::assemble_data_part_3(void)
{
    const toml::value data_part_3{
        { "backend.semtech_udp",
          { { "fake_rx_time", this->fake_rx_time },
            { "skip_crc_check", this->skip_crc_check },
            { "udp_bind", (this->udp_ip) + string(":") + to_string(this->udp_port) } } },
        {
            "backend",
            { { "type", this->backend_type } },
        },
    };
    this->outfile << data_part_3 << std::endl;
    const toml::value data_part_3_{
        { "backend.basic_station",
          { { "frequency_max", this->frequency_max },
            { "frequency_min", this->frequency_min },
            { "region", this->region },
            { "write_timeout", parse_json_sec_to_string(this->write_timeout) },
            { "read_timeout", parse_json_sec_to_string(this->read_timeout) },
            { "ping_interval", parse_json_sec_to_string(this->ping_interval) },
            { "ca_cert", this->bs_ca_cert },
            { "tls_key", this->bs_tls_key },
            { "tls_cert", this->bs_tls_cert },
            { "bind", (this->bs_ip) + string(":") + to_string(this->bs_port) } } },
    };
    this->outfile << data_part_3_ << std::endl;
}

void BridgeToml::assemble_data_part_4(void)
{
    const toml::value data_part_4{
        { "integration.mqtt",
          { { "max_reconnect_interval", parse_json_sec_to_string(this->max_reconnect_interval) },
            { "command_topic_template", this->command_topic_template },
            { "event_topic_template", this->event_topic_template } } },

        { "integration.mqtt.auth", { { "type", this->mqtt_auth_type } } },
        { "integration", { { "marshaler", this->marshaler } } },
    };

    this->outfile << data_part_4 << std::endl;
}

void BridgeToml::assemble_data_part_5(void)
{
    const toml::value data_part_5{
        { "integration.mqtt.auth.gcp_cloud_iot_core",
          {
              { "jwt_key_file", this->gcp_jwt_key_file },
              { "jwt_expiration", parse_json_sec_to_string(this->gcp_jwt_expiration) },
              { "registry_id", this->gcp_registry_id },
              { "cloud_region", this->gcp_cloud_region },
              { "project_id", this->gcp_project_id },
              { "device_id", this->gcp_device_id },
              { "server", this->gcp_server },
          } },

        { "integration.mqtt.auth.azure_iot_hub",
          {
              { "tls_key", this->azure_tls_key },
              { "tls_cert", this->azure_tls_cert },
              { "hostname", this->azure_hostname },
              { "device_id", this->azure_device_id },
              { "sas_token_expiration",
                parse_json_sec_to_string(this->azure_sas_token_expiration) },
              { "device_connection_string", this->azure_device_connection_string },
          } },

        { "integration.mqtt.auth.generic",
          {
              { "tls_pass_phrase", this->generic_pass_phrase },
              { "tls_key", this->generic_tls_key },
              { "tls_cert", this->generic_tls_cert },
              { "ca_cert", this->generic_ca_cert },
              { "client_id", this->generic_client_id },
              { "clean_session", this->generic_clean_session },
              { "qos", this->generic_qos },
              { "password", this->generic_password },
              { "username", this->generic_username },
              { "server", string(this->generic_ip) + string(":") + to_string(this->generic_port) },
          } },
    };
    this->outfile << data_part_5 << std::endl;
}

void BridgeToml::assemble_data_part_6(void)
{
    const toml::value data_part_6{
        { "metrics.prometheus", { { "bind", "" }, { "endpoint_enabled", false } } },
        { "metrics", { { "type", "" } } },

    };
    this->outfile << data_part_6 << std::endl;
}

void BridgeToml::assemble_data_part_7(void)
{
    const toml::value data_part_7{
        { "meta_data.dynamic.commands", { { "commands", "" } } },
        { "meta_data.dynamic",
          { { "execution_interval", "1m0s" }, { "max_execution_duration", "1s" } } },
        { "meta_data.static", { { "serial_number", "" } } },
        { "meta_data", { { "type", "" } } },
    };

    this->outfile << data_part_7 << std::endl;
}

void BridgeToml::assemble_data_part_8(void)
{
    const toml::value data_part_8{
        { "commands.prometheus", { { "bind", "" }, { "endpoint_enabled", false } } },
        { "commands", { { "type", "" } } },

    };
    this->outfile << data_part_8 << std::endl;
}

void BridgeToml::output_formatted_toml_file(void)
{
    // 覆盖写
    this->infile.open(TMP_FILE_NAME);
    // 打开文件、没有就新建、有则清空再覆盖
    ofstream format_out(BRIDGE_CONF_DEFAULT, ios::trunc);
    string   string_line;
    int      idx;
    while (getline(this->infile, string_line)) {
        idx = string_line.find("[\"");
        if (idx != string::npos) {
            idx = string_line.find(".");
            if (idx != string::npos) {
                string_line.erase(remove(string_line.begin(), string_line.end(), '\"'),
                                  string_line.end());
            }
        }
        idx = string_line.find("[]");
        if (idx != string::npos) {
            string_line = "# " + string_line;
        }
        if (string_line == "type = \"\"") {
            continue;
        }
        format_out << string_line << std::endl;
    }
    format_out.close();
    this->infile.close();
    this->outfile.close();
    remove(TMP_FILE_NAME);
}

void BridgeToml::update_local_toml_content(void)
{
    this->open_output_file();
    this->assemble_data_part_1();
    this->assemble_data_part_2();
    this->assemble_data_part_3();
    this->assemble_data_part_4();
    this->assemble_data_part_5();
#ifdef USE_EXPAND
    this->assemble_data_part_6();
    this->assemble_data_part_7();
    this->assemble_data_part_8();
#endif
    this->output_formatted_toml_file();
}

int main_toml_get(string option, string input_json, string &rsp)
{
    auto        rc     = -1;
    BridgeToml *p_toml = new BridgeToml();
    if (p_toml == NULL) {
        rsp = "Malloc mem failed.\n";
        return -1;
    }
    rc = main_parser_get(p_toml, option, input_json, rsp);
    delete p_toml;
    return rc;
}

int main_toml_set(string option, string input_json, string &rsp)
{
    auto        rc     = -1;
    BridgeToml *p_toml = new BridgeToml();
    if (p_toml == NULL) {
        rsp = "Malloc mem failed.\n";
        return -1;
    }
    rc = main_parser_set(p_toml, option, input_json, rsp);
    delete p_toml;
    return rc;
}
