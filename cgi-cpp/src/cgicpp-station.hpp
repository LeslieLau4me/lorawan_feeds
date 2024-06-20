/**
 * @file
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 */

#ifndef _CGICPP_STATION_HPP_
#define _CGICPP_STATION_HPP_
#include "cgicpp-lorawan-mode.hpp"
#include "cgicpp-parser-base.hpp"
#include "easylogging++.h"
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
using namespace std;
using json = nlohmann::json;

#define STATION_CONF_DEFAULT "/etc/basicstation/station.conf"

#define RADIO_COMFIG "radio_conf"
#define STATION_CONF "station_conf"
#define RADIO_COMMON "radio_common"
#define RADIO_0      "radio_0"
#define RADIO_1      "radio_1"

#define BODY "body"

#define BS_RADIO_COMMON_GET "cgicpp-bs-radio-common-get"
#define BS_RADIO_0_GET      "cgicpp-bs-radio0-get"
#define BS_RADIO_1_GET      "cgicpp-bs-radio1-get"
#define BS_STATION_GET      "cgicpp-bs-station-get"
#define BS_ALL_GET          "cgicpp-bs-all-get"

#define BS_RADIO_COMMON_SET "cgicpp-bs-radio-common-set"
#define BS_RADIO_0_SET      "cgicpp-bs-radio0-set"
#define BS_RADIO_1_SET      "cgicpp-bs-radio1-set"
#define BS_STATION_SET      "cgicpp-bs-station-set"
#define BS_TC_URI_SET       "cgicpp-bs-tc-uri-set"
#define BS_TC_KEY_SET       "cgicpp-bs-tc-key-set"
#define BS_CUPS_URI_SET     "cgicpp-bs-cups-uri-set"
#define BS_CUPS_KEY_SET     "cgicpp-bs-cups-key-set"
#define BS_ALL_SET          "cgicpp-bs-all-set"

class LoRaStation : public CgiParserBase
{
  private:
    bool   b_has_except = false;
    string err_msg;
    // 特定于平台的spi设备
    string device;
    bool   pps            = true;
    bool   lorawan_public = true;
    // radio 1提供时钟源给集中器
    uint8_t clksrc      = 0;
    bool    full_duplex = false;

    string   radio_0_type;
    uint32_t radio_0_feq;
    // 天线增益
    uint8_t antenna_gain_0;
    double  rssi_offset_0 = -215.40000;

    bool tx_enable_0 = true;

    string   radio_1_type;
    uint32_t radio_1_feq;
    // 天线增益
    uint8_t antenna_gain_1;
    double  rssi_offset_1 = -215.40000;

    bool tx_enable_1 = false;

    string   routerid;
    string   radio_init;
    string   log_file;
    string   log_level;
    uint32_t log_size;
    uint8_t  log_rotate;

    ifstream json_ifstream;
    ofstream json_ofstream;
    json     local_json;
    json     upload_json;
    json     remote_json;

    string remote_str;
    string serialized_str;
    typedef void (LoRaStation::*json_option_cb)(void);
    void parse_radio_common_config(void);
    void parse_radio_0_config(void);
    void parse_radio_1_config(void);
    void parse_station_config(void);
    void generate_serialize_json(void);

    void parse_remote_json_string(void);
    void parse_remote_radio_common_config(void);
    void parse_remote_radio_0_config(void);
    void parse_remote_radio_1_config(void);
    void parse_remote_station_config(void);
    void parse_remote_tc_uri_config(void);
    void parse_remote_tc_key_config(void);

    void parse_remote_cups_uri_config(void);
    void parse_remote_cups_key_config(void);

    void output_modyfied_json_file(void);

    map<string, json_option_cb> parse_local_cb_map = {
        { BS_RADIO_COMMON_GET, &LoRaStation::parse_radio_common_config },
        { BS_RADIO_0_GET, &LoRaStation::parse_radio_0_config },
        { BS_RADIO_1_GET, &LoRaStation::parse_radio_1_config },
        { BS_STATION_GET, &LoRaStation::parse_station_config }
    };

    map<string, json_option_cb> parse_remote_cb_map = {
        { BS_RADIO_COMMON_SET, &LoRaStation::parse_remote_radio_common_config },
        { BS_RADIO_0_SET, &LoRaStation::parse_remote_radio_0_config },
        { BS_RADIO_1_SET, &LoRaStation::parse_remote_radio_1_config },
        { BS_STATION_SET, &LoRaStation::parse_remote_station_config },
        { BS_TC_URI_SET, &LoRaStation::parse_remote_tc_uri_config },
        { BS_TC_KEY_SET, &LoRaStation::parse_remote_tc_key_config },
        { BS_CUPS_URI_SET, &LoRaStation::parse_remote_cups_uri_config },
        { BS_CUPS_KEY_SET, &LoRaStation::parse_remote_cups_key_config }
    };

  public:
    LoRaStation() : CgiParserBase("LoRaStation")
    {
        json_ifstream.open(STATION_CONF_DEFAULT);
        json_ifstream >> this->local_json;
        json_ifstream.close();
    }
    ~LoRaStation();
    void parse_local_for_each(void);
    void parse_local_for_one(string opt);
    void get_upload_json(string &str);

    void set_remote_string(string input_string);
    void set_local_for_each(void);
    void set_local_for_one(string opt);

    bool get_exception_status(void);
    void get_raise_err_msg(string &err);
};

int main_station_get(string option, string input_json, string &rsp);
int main_station_set(string option, string input_json, string &rsp);
#endif