/**
 * @file
 * @brief  获取/修改网关LoRaWAN Bridge的mqtt主题
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 *
 * @details LoRaWAN Bridge的主题格式为{gateway_eui}/event/*，返回json格式的数据内容和运行结果
 */

#include "cgicpp-bridge-topic.hpp"
#include "cgicpp-lorawan-mode.hpp"
#include "cgicpp-toml.hpp"
#include "easylogging++.h"
#include <unistd.h>

using namespace std;
using json = nlohmann::json;

int get_tolower_gateway_eui(string &eui)
{
    char gateway_eui[MAX_GATEWAY_ID + 1] = { 0 };
    if (generate_gateway_id_by_mac(gateway_eui) < 0) {
        LOG(ERROR) << "Failed to get eth mac." << std::endl;
        return -1;
    }
    eui = gateway_eui;
    std::transform(
        eui.begin(), eui.end(), eui.begin(), [](unsigned char c) { return std::tolower(c); });
    return 0;
}

int generate_bridge_mqtt_topic(string &rsp)
{
    ifstream json_ifstream;
    json     local_json;
    string   eui;
    try {
        json_ifstream.open(BRIDGE_TOPIC_CONF_DEFAULT);
        json_ifstream >> local_json;
        json_ifstream.close();
        eui = local_json["gateway_eui"];
    } catch (const std::exception &e) {
        LOG(ERROR) << e.what() << '\n';
    }

    string str_eui;
    if (get_tolower_gateway_eui(str_eui) < 0) {
        LOG(ERROR) << "Failed to get tolower eui.";
        return -1;
    }
    if (eui != str_eui) {
        json setting_json;
        setting_json["gateway_eui"] = str_eui;
        setting_json["topic_pub_rxpk"] =
            string("gateway/") + str_eui + string("/event/") + string("up");
        setting_json["topic_pub_downlink"] =
            string("gateway/") + str_eui + string("/event/") + string("down");
        setting_json["topic_pub_downlink_ack"] =
            string("gateway/") + str_eui + string("/event/") + string("ack");
        setting_json["topic_pub_gateway_stat"] =
            string("gateway/") + str_eui + string("/event/") + string("stat");
        setting_json["topic_sub_txpk"] =
            string("gateway/") + str_eui + string("/event/") + string("tx");
        rsp = setting_json.dump(4);
        ofstream json_ofstream;
        json_ofstream.open(BRIDGE_TOPIC_CONF_DEFAULT);
        json_ofstream << setting_json.dump(4) << endl;
        json_ofstream.close();
    } else {
        rsp = local_json.dump(4);
    }
    return 0;
}

int set_bridge_mqtt_topic(string input_json, string &rsp)
{
    ifstream json_ifstream;
    ofstream json_ofstream;
    json     local_json;
    json     remote_json;

    string topic_pub_rxpk;
    string topic_pub_downlink;
    string topic_pub_downlink_ack;
    string topic_pub_gateway_stat;
    string topic_sub_txpk;
    string str_eui;
    if (get_tolower_gateway_eui(str_eui) < 0) {
        LOG(ERROR) << "Failed to get tolower eui.";
        return -1;
    }
    /* 主题格式: eui/event */
    string format = str_eui + "/event";
    auto   rc     = -1;
    try {
        json_ifstream.open(BRIDGE_TOPIC_CONF_DEFAULT);
        json_ifstream >> local_json;
        json_ifstream.close();

        remote_json = json::parse(input_json);
        remote_json["body"]["topic_pub_rxpk"].get_to(topic_pub_rxpk);
        remote_json["body"]["topic_pub_downlink"].get_to(topic_pub_downlink);
        remote_json["body"]["topic_pub_downlink_ack"].get_to(topic_pub_downlink_ack);
        remote_json["body"]["topic_pub_gateway_stat"].get_to(topic_pub_gateway_stat);
        remote_json["body"]["topic_sub_txpk"].get_to(topic_sub_txpk);

        if (topic_pub_rxpk.find(format) == string::npos ||
            topic_pub_downlink.find(format) == string::npos ||
            topic_pub_downlink_ack.find(format) == string::npos ||
            topic_pub_gateway_stat.find(format) == string::npos ||
            topic_sub_txpk.find(format) == string::npos) {
            rsp = "The Topic format must include the {gateway_eui}/event field.";
            return -1;
        }

        if (topic_sub_txpk == topic_pub_rxpk || topic_sub_txpk == topic_pub_downlink ||
            topic_sub_txpk == topic_pub_downlink_ack || topic_sub_txpk == topic_pub_gateway_stat) {
            rsp = "The Topic used for tx must be diffrent from other topics.";
            return -1;
        }

        local_json["topic_pub_rxpk"]         = topic_pub_rxpk;
        local_json["topic_pub_downlink"]     = topic_pub_downlink;
        local_json["topic_pub_downlink_ack"] = topic_pub_downlink_ack;
        local_json["topic_pub_gateway_stat"] = topic_pub_gateway_stat;
        local_json["topic_sub_txpk"]         = topic_sub_txpk;

        json_ofstream.open(BRIDGE_TOPIC_CONF_DEFAULT);
        json_ofstream << local_json.dump(4) << endl;
        json_ofstream.close();
        rc = 0;
    } catch (const std::exception &e) {
        LOG(ERROR) << e.what();
        rsp = "Failed to set Bridge Topic.";
        rc  = -1;
    }
    return rc;
}

int main_bridge_topic_get(string option, string input_json, string &rsp)
{
    return generate_bridge_mqtt_topic(rsp);
}

int main_bridge_topic_set(string option, string input_json, string &rsp)
{
    return set_bridge_mqtt_topic(input_json, rsp);
}