/**
 * @file
 * @brief  获取/修改网关LoRaWAN 数据包过滤信息
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 *
 * @details 获取LoRaWAN地区数据包的过滤信息，返回json格式的数据内容和运行结果
 */

#include "cgicpp-lorawan-filter.hpp"
#include "cgicpp-parser-base.hpp"
#include "cgicpp-toml.hpp"
#include "easylogging++.h"
#include <unistd.h>
#include <vector>

using namespace std;
using json = nlohmann::json;

class LoRaWANFilter : public CgiParserBase
{
  private:
    ifstream json_ifstream;
    ofstream json_ofstream;
    json     local_json;
    json     upload_json;
    json     remote_json;

    string         remote_str;
    string         serialized_str;
    vector<string> dev_eui_vec;
    bool           filter_enable = false;

    void parse_remote_json_string(void);
    void parse_remote_filter_enable(void);
    void parse_remote_deveui_white_list(void);
    void generate_serialize_json(void);
    void parse_local_filter_enable(void);
    void parse_local_deveui_white_list(void);

    void output_modyfied_json_file(void);

  public:
    LoRaWANFilter() : CgiParserBase("lorawan filter")
    {
        json_ifstream.open(LORAWAN_FILTER_CONF_DEFAULT);
        json_ifstream >> this->local_json;
        json_ifstream.close();
    }
    ~LoRaWANFilter();
    void set_remote_string(string input_string);
    void set_local_for_each(void);
    void parse_local_for_each(void);
    void get_upload_json(string &str);
};

void LoRaWANFilter::generate_serialize_json(void)
{
    this->serialized_str = this->upload_json.dump(4);
}

void LoRaWANFilter::set_local_for_each(void)
{
    this->parse_remote_json_string();
    this->parse_remote_filter_enable();
    this->parse_remote_deveui_white_list();
    this->output_modyfied_json_file();
}

void LoRaWANFilter::parse_local_for_each(void)
{
    this->parse_local_filter_enable();
    this->parse_local_deveui_white_list();
    this->generate_serialize_json();
}

void LoRaWANFilter::parse_remote_json_string(void)
{
    this->remote_json = json::parse(this->remote_str);
}

void LoRaWANFilter::set_remote_string(string input_string)
{
    this->remote_str = input_string;
}

void LoRaWANFilter::get_upload_json(string &str)
{
    str = this->serialized_str;
}

void LoRaWANFilter::output_modyfied_json_file(void)
{
    this->json_ofstream.open(LORAWAN_FILTER_CONF_DEFAULT);
    this->json_ofstream << local_json.dump(4) << endl;
    this->json_ofstream.close();
}

void LoRaWANFilter::parse_local_filter_enable(void)
{
    try {
        this->local_json["filter_enable"].get_to(this->filter_enable);
    } catch (const std::exception &e) {
        LOG(ERROR) << e.what() << endl;
    }
    this->upload_json["filter_enable"] = this->filter_enable;
}

void LoRaWANFilter::parse_remote_filter_enable(void)
{
    this->remote_json["body"]["filter_enable"].get_to(this->filter_enable);
    this->local_json["filter_enable"] = this->filter_enable;
}

void LoRaWANFilter::parse_remote_deveui_white_list(void)
{
    if (this->filter_enable == true) {
        this->remote_json["body"]["white_list"].get_to(this->dev_eui_vec);
        this->local_json["white_list"] = this->dev_eui_vec;
    }
}

void LoRaWANFilter::parse_local_deveui_white_list(void)
{
    if (this->local_json.contains("white_list")) {
        this->local_json["white_list"].get_to(this->dev_eui_vec);
    }
    this->upload_json["white_list"] = this->dev_eui_vec;
}

LoRaWANFilter::~LoRaWANFilter() {}

int main_lorawan_filter_get(string option, string input_json, string &rsp)
{
    auto           rc     = -1;
    LoRaWANFilter *filter = new LoRaWANFilter();
    if (filter == NULL) {
        return -1;
    }
    rc = main_parser_get(filter, option, input_json, rsp);
    delete filter;
    return rc;
}

int main_lorawan_filter_set(string option, string input_json, string &rsp)
{
    auto           rc     = -1;
    LoRaWANFilter *filter = new LoRaWANFilter();
    if (filter == NULL) {
        return -1;
    }
    rc = main_parser_set(filter, option, input_json, rsp);
    delete filter;
    return rc;
}
