/**
 * @file
 * @brief  对Basics Station 的json类型配置文件进行读写操作
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 *
 * @details 识别具体的命令类型执行相应的回调、对配置文件进行修改或读取，返回json格式的数据内容和运行结果
 */

#include "cgicpp-station.hpp"

using namespace std;
using json = nlohmann::json;

void LoRaStation::set_remote_string(string input_string)
{
    this->remote_str = input_string;
}

void LoRaStation::output_modyfied_json_file(void)
{
    this->json_ofstream.open(STATION_CONF_DEFAULT);
    this->json_ofstream << local_json.dump(4) << endl;
    this->json_ofstream.close();
}

void LoRaStation::set_local_for_one(string opt)
{
    this->parse_remote_json_string();
    if (this->parse_remote_cb_map.count(opt)) {
        (this->*parse_remote_cb_map[opt])();
    }
    this->output_modyfied_json_file();
}

void LoRaStation::set_local_for_each(void)
{
    this->parse_remote_json_string();
    this->parse_remote_radio_common_config();
    this->parse_remote_radio_0_config();
    this->parse_remote_radio_1_config();
    this->parse_remote_station_config();
    this->output_modyfied_json_file();
}

void LoRaStation::parse_remote_json_string(void)
{
    this->remote_json = json::parse(this->remote_str);
}

void LoRaStation::get_upload_json(string &str)
{
    str = this->serialized_str;
}

void LoRaStation::generate_serialize_json(void)
{
    serialized_str = this->upload_json.dump(4);
}

void LoRaStation::parse_local_for_each(void)
{
    this->parse_radio_common_config();
    this->parse_radio_0_config();
    this->parse_radio_1_config();
    this->parse_station_config();
    this->generate_serialize_json();
}
void LoRaStation::parse_local_for_one(string opt)
{
    if (this->parse_local_cb_map.count(opt)) {
        (this->*parse_local_cb_map[opt])();
    }
    this->generate_serialize_json();
}

void LoRaStation::parse_radio_common_config(void)
{
    local_json[RADIO_COMFIG]["device"].get_to(this->device);
    LOG(INFO) << "device is " << this->device;
    upload_json[RADIO_COMMON]["device"] = this->device;

    local_json[RADIO_COMFIG]["pps"].get_to(this->pps);
    LOG(INFO) << "pps is " << this->pps;
    upload_json[RADIO_COMMON]["pps"] = this->pps;

    local_json[RADIO_COMFIG]["lorawan_public"].get_to(this->lorawan_public);
    LOG(INFO) << "lorawan_public is " << this->lorawan_public;
    upload_json[RADIO_COMMON]["lorawan_public"] = this->lorawan_public;

    local_json[RADIO_COMFIG]["clksrc"].get_to(this->clksrc);
    LOG(INFO) << "clksrc is " << this->clksrc;
    upload_json[RADIO_COMMON]["clksrc"] = this->clksrc;

    local_json[RADIO_COMFIG]["full_duplex"].get_to(this->full_duplex);
    LOG(INFO) << "full_duplex is " << this->full_duplex;
    upload_json[RADIO_COMMON]["full_duplex"] = this->full_duplex;
}

void LoRaStation::parse_remote_radio_common_config(void)
{
    remote_json[BODY][RADIO_COMMON]["device"].get_to(this->device);
    LOG(INFO) << "remote device is " << this->device;
    local_json[RADIO_COMFIG]["device"] = this->device;

    remote_json[BODY][RADIO_COMMON]["pps"].get_to(this->pps);
    LOG(INFO) << "remote pps is " << this->pps;
    local_json[RADIO_COMFIG]["pps"] = this->pps;

    remote_json[BODY][RADIO_COMMON]["lorawan_public"].get_to(this->lorawan_public);
    LOG(INFO) << "remote lorawan_public is " << this->lorawan_public;
    local_json[RADIO_COMFIG]["lorawan_public"] = this->lorawan_public;

    remote_json[BODY][RADIO_COMMON]["clksrc"].get_to(this->clksrc);
    LOG(INFO) << "remote clksrc is " << this->clksrc;
    local_json[RADIO_COMFIG]["clksrc"] = this->clksrc;

    remote_json[BODY][RADIO_COMMON]["full_duplex"].get_to(this->full_duplex);
    LOG(INFO) << "remote full_duplex is " << this->full_duplex;
    local_json[RADIO_COMFIG]["full_duplex"] = this->full_duplex;
}

void LoRaStation::parse_radio_0_config(void)
{
    local_json[RADIO_COMFIG]["radio_0"]["type"].get_to(this->radio_0_type);
    LOG(INFO) << "radio_0 is " << this->radio_0_type;
    upload_json[RADIO_0]["type"] = this->radio_0_type;

    local_json[RADIO_COMFIG]["radio_0"]["freq"].get_to(this->radio_0_feq);
    LOG(INFO) << "radio_0_feq is " << this->radio_0_feq;
    upload_json[RADIO_0]["freq"] = this->radio_0_feq;

    local_json[RADIO_COMFIG]["radio_0"]["antenna_gain"].get_to(this->antenna_gain_0);
    LOG(INFO) << "antenna_gain_0 is " << this->antenna_gain_0;
    upload_json[RADIO_0]["antenna_gain"] = this->antenna_gain_0;

    local_json[RADIO_COMFIG]["radio_0"]["rssi_offset"].get_to(this->rssi_offset_0);
    LOG(INFO) << "rssi_offset_0 is " << this->rssi_offset_0;
    upload_json[RADIO_0]["rssi_offset"] = this->rssi_offset_0;

    local_json[RADIO_COMFIG]["radio_0"]["tx_enable"].get_to(this->tx_enable_0);
    LOG(INFO) << "tx_enable_0 is " << this->tx_enable_0;
    upload_json[RADIO_0]["tx_enable"] = this->tx_enable_0;
}

void LoRaStation::parse_remote_radio_0_config(void)
{
    remote_json[BODY][RADIO_0]["type"].get_to(this->radio_0_type);
    LOG(INFO) << "remote radio_0 is " << this->radio_0_type;
    local_json[RADIO_COMFIG][RADIO_0]["type"] = this->radio_0_type;

    remote_json[BODY][RADIO_0]["freq"].get_to(this->radio_0_feq);
    LOG(INFO) << "remote radio_0_feq is " << this->radio_0_feq;
    local_json[RADIO_COMFIG][RADIO_0]["freq"] = this->radio_0_feq;

    remote_json[BODY][RADIO_0]["antenna_gain"].get_to(this->antenna_gain_0);
    LOG(INFO) << "remote antenna_gain_0 is " << this->antenna_gain_0;
    local_json[RADIO_COMFIG][RADIO_0]["antenna_gain"] = this->antenna_gain_0;

    remote_json[BODY][RADIO_0]["rssi_offset"].get_to(this->rssi_offset_0);
    LOG(INFO) << "remote rssi_offset_0 is " << this->rssi_offset_0;
    local_json[RADIO_COMFIG][RADIO_0]["rssi_offset"] = this->rssi_offset_0;

    remote_json[BODY][RADIO_0]["tx_enable"].get_to(this->tx_enable_0);
    LOG(INFO) << "remote tx_enable_0 is " << this->tx_enable_0;
    local_json[RADIO_COMFIG][RADIO_0]["tx_enable"] = this->tx_enable_0;
}

void LoRaStation::parse_radio_1_config(void)
{
    local_json[RADIO_COMFIG][RADIO_1]["type"].get_to(this->radio_1_type);
    LOG(INFO) << "radio_1_type is " << this->radio_1_type;
    upload_json[RADIO_1]["type"] = this->radio_1_type;

    local_json[RADIO_COMFIG][RADIO_1]["freq"].get_to(this->radio_1_feq);
    LOG(INFO) << "radio_1_feq is " << this->radio_1_feq;
    upload_json[RADIO_1]["freq"] = this->radio_1_feq;

    local_json[RADIO_COMFIG][RADIO_1]["antenna_gain"].get_to(this->antenna_gain_1);
    LOG(INFO) << "antenna_gain_1 is " << this->antenna_gain_1;
    upload_json[RADIO_1]["antenna_gain"] = this->antenna_gain_1;

    local_json[RADIO_COMFIG][RADIO_1]["rssi_offset"].get_to(this->rssi_offset_1);
    LOG(INFO) << "rssi_offset_1 is " << this->rssi_offset_1;
    upload_json[RADIO_1]["rssi_offset"] = this->rssi_offset_1;

    local_json[RADIO_COMFIG][RADIO_1]["tx_enable"].get_to(this->tx_enable_1);
    LOG(INFO) << "tx_enable_1 is " << this->tx_enable_1;
    upload_json[RADIO_1]["tx_enable"] = this->tx_enable_1;
}

void LoRaStation::parse_remote_radio_1_config(void)
{
    remote_json[BODY][RADIO_1]["type"].get_to(this->radio_1_type);
    LOG(INFO) << "remote radio_1_type is " << this->radio_1_type;
    local_json[RADIO_COMFIG][RADIO_1]["type"] = this->radio_1_type;

    remote_json[BODY][RADIO_1]["freq"].get_to(this->radio_1_feq);
    LOG(INFO) << "remote radio_1_feq is " << this->radio_1_feq;
    local_json[RADIO_COMFIG][RADIO_1]["freq"] = this->radio_1_feq;

    remote_json[BODY][RADIO_1]["antenna_gain"].get_to(this->antenna_gain_1);
    LOG(INFO) << "remote antenna_gain_1 is " << this->antenna_gain_1;
    local_json[RADIO_COMFIG][RADIO_1]["antenna_gain"] = this->antenna_gain_1;

    remote_json[BODY][RADIO_1]["rssi_offset"].get_to(this->rssi_offset_1);
    LOG(INFO) << "remote rssi_offset_1 is " << this->rssi_offset_1;
    local_json[RADIO_COMFIG][RADIO_1]["rssi_offset"] = this->rssi_offset_1;

    remote_json[BODY][RADIO_1]["tx_enable"].get_to(this->tx_enable_1);
    LOG(INFO) << "remote tx_enable_1 is " << this->tx_enable_1;
    local_json[RADIO_COMFIG][RADIO_1]["tx_enable"] = this->tx_enable_1;
}

void LoRaStation::parse_station_config(void)
{
    local_json[STATION_CONF]["routerid"].get_to(this->routerid);
    LOG(INFO) << "routerid is " << this->routerid;
    upload_json[STATION_CONF]["routerid"] = this->routerid;

    local_json[STATION_CONF]["radio_init"].get_to(this->radio_init);
    LOG(INFO) << "radio_init is " << this->radio_init;
    upload_json[STATION_CONF]["radio_init"] = this->radio_init;

    local_json[STATION_CONF]["log_file"].get_to(this->log_file);
    LOG(INFO) << "log_file is " << this->log_file;
    upload_json[STATION_CONF]["log_file"] = this->log_file;

    local_json[STATION_CONF]["log_level"].get_to(this->log_level);
    LOG(INFO) << "log_level is " << this->log_level;
    upload_json[STATION_CONF]["log_level"] = this->log_level;

    local_json[STATION_CONF]["log_size"].get_to(this->log_size);
    LOG(INFO) << "log_size is " << this->log_size;
    upload_json[STATION_CONF]["log_size"] = this->log_size;

    local_json[STATION_CONF]["log_rotate"].get_to(this->log_rotate);
    LOG(INFO) << "log_rotate is " << this->log_rotate;
    upload_json[STATION_CONF]["log_rotate"] = this->log_rotate;
}

void LoRaStation::parse_remote_station_config(void)
{
    remote_json[BODY][STATION_CONF]["routerid"].get_to(this->routerid);
    LOG(INFO) << "remote routerid is " << this->routerid;
    local_json[STATION_CONF]["routerid"] = this->routerid;

    remote_json[BODY][STATION_CONF]["radio_init"].get_to(this->radio_init);
    LOG(INFO) << "remote radio_init is " << this->radio_init;
    local_json[STATION_CONF]["radio_init"] = this->radio_init;

    remote_json[BODY][STATION_CONF]["log_file"].get_to(this->log_file);
    LOG(INFO) << "remote log_file is " << this->log_file;
    local_json[STATION_CONF]["log_file"] = this->log_file;

    remote_json[BODY][STATION_CONF]["log_level"].get_to(this->log_level);
    LOG(INFO) << "remote log_level is " << this->log_level;
    local_json[STATION_CONF]["log_level"] = this->log_level;

    remote_json[BODY][STATION_CONF]["log_size"].get_to(this->log_size);
    LOG(INFO) << "remote log_size is " << this->log_size;
    local_json[STATION_CONF]["log_size"] = this->log_size;

    remote_json[BODY][STATION_CONF]["log_rotate"].get_to(this->log_rotate);
    LOG(INFO) << "remote log_rotate is " << this->log_rotate;
    local_json[STATION_CONF]["log_rotate"] = this->log_rotate;
}

// Basicstation LNS 模式 uri 解析并以文件形式保存到指定路径
void LoRaStation::parse_remote_tc_uri_config(void)
{
    string tc_uri;
    auto   uri_dir = "/etc/ssl/gw/tc_uri";
    if (access(uri_dir, F_OK) == -1) {
        if (mkdir(uri_dir, S_IRUSR | S_IWUSR) < 0) {
            LOG(ERROR) << "Failed to create dir: " << uri_dir;
        }
    } else {
        chmod(uri_dir, 0600);
        LOG(INFO) << "target dir exsit: " << uri_dir;
    }
    if (this->remote_json[BODY].contains("tc_uri")) {
        remote_json[BODY]["tc_uri"].get_to(tc_uri);
        ofstream out;
        out.open(TC_URI, ios::out);
        out << tc_uri;
        out.close();
    }
}

// Basicstation LNS 模式 token 解析并以文件形式保存到指定路径
void LoRaStation::parse_remote_tc_key_config(void)
{
    string tc_key;
    auto   key_dir = "/etc/ssl/gw/tc_token";
    if (access(key_dir, F_OK) == -1) {
        if (mkdir(key_dir, S_IRUSR | S_IWUSR) < 0) {
            LOG(ERROR) << "Failed to create dir: " << key_dir;
        }
    } else {
        chmod(key_dir, 0600);
        LOG(INFO) << "target dir exsit: " << key_dir;
    }
    if (this->remote_json[BODY].contains("tc_key")) {
        remote_json[BODY]["tc_key"].get_to(tc_key);
        if (tc_key.find("Authorization: ") == string::npos) {
            tc_key = "Authorization: " + tc_key;
        }
        ofstream out;
        out.open(TC_CLIENT_TOKEN, ios::out);
        out << tc_key;
        out.close();
    }
}

// Basicstation CUPS 模式 uri 解析并以文件形式保存到指定路径
void LoRaStation::parse_remote_cups_uri_config(void)
{
    string cups_uri;
    auto   uri_dir = "/etc/ssl/gw/cups_boot_uri";
    if (access(uri_dir, F_OK) == -1) {
        if (mkdir(uri_dir, S_IRUSR | S_IWUSR) < 0) {
            LOG(ERROR) << "Failed to create dir: " << uri_dir;
        }
    } else {
        chmod(uri_dir, 0600);
        LOG(INFO) << "target dir exsit: " << uri_dir;
    }
    if (this->remote_json[BODY].contains("cups_uri")) {
        remote_json[BODY]["cups_uri"].get_to(cups_uri);
        ofstream out;
        out.open(CUPS_URI, ios::out);
        out << cups_uri;
        out.close();
    }
}

// Basicstation CUPS 模式 token 解析并以文件形式保存到指定路径
void LoRaStation::parse_remote_cups_key_config(void)
{
    string cups_key;
    auto   key_dir = "/etc/ssl/gw/cups_boot_token";
    if (access(key_dir, F_OK) == -1) {
        if (mkdir(key_dir, S_IRUSR | S_IWUSR) < 0) {
            LOG(ERROR) << "Failed to create dir: " << key_dir;
        }
    } else {
        chmod(key_dir, 0600);
        LOG(INFO) << "target dir exsit: " << key_dir;
    }
    if (this->remote_json[BODY].contains("cups_key")) {
        remote_json[BODY]["cups_key"].get_to(cups_key);
        if (cups_key.find("Authorization: ") == string::npos) {
            cups_key = "Authorization: " + cups_key;
        }
        ofstream out;
        out.open(CUPS_CLIENT_TOKEN, ios::out);
        out << cups_key;
        out.close();
    }
}

bool LoRaStation::get_exception_status(void)
{
    return this->b_has_except;
}

void LoRaStation::get_raise_err_msg(string &err)
{
    err = this->err_msg;
}

LoRaStation::~LoRaStation() {}

int main_station_get(string option, string input_json, string &rsp)
{
    auto         rc      = -1;
    LoRaStation *station = new LoRaStation();
    if (station == NULL) {
        return -1;
    }
    rc = main_parser_get(station, option, input_json, rsp);
    delete station;
    return rc;
}

int main_station_set(string option, string input_json, string &rsp)
{
    auto         rc      = -1;
    LoRaStation *station = new LoRaStation();
    if (station == NULL) {
        return -1;
    }
    rc = main_parser_set(station, option, input_json, rsp);
    if (rc < 0) {
        goto end;
    }
    if (station->get_exception_status()) {
        station->get_raise_err_msg(rsp);
        rc = -1;
    }
end:
    delete station;
    return rc;
}