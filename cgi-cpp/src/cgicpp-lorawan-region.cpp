/**
 * @file
 * @brief  获取/修改网关LoRaWAN 地区频段计划
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 *
 * @details 获取LoRaWAN地区频段计划：US915/EU868/CN490，返回json格式的数据内容和运行结果
 */

#include "cgicpp-lorawan-region.hpp"
#include "cgicpp-toml.hpp"
#include "easylogging++.h"
#include <unistd.h>

using namespace std;
using json = nlohmann::json;

static void choose_global_conf_region(const string region)
{
    string file_name = GLOBAL_CONF_SX1250_COMMON + string(".") + region;
    string cmd       = string("cp ") + file_name + string(" ") + GLOBAL_CONF_SX1250 + string(" &");
    system(cmd.c_str());

}

int main_lorawan_region_get(string option, string input_json, string &rsp)
{
    string   region;
    ifstream json_ifstream;
    json     local_json;
    json     upload_json;

    auto rc = -1;
    try {
        json_ifstream.open(LORAWAN_REGION_CONF_DEFAULT);
        json_ifstream >> local_json;
        json_ifstream.close();

        local_json["lorawan_region"].get_to(region);
        upload_json["region"] = region;
        rsp                   = upload_json.dump(4);
        rc                    = 0;
    } catch (const std::exception &e) {
        LOG(ERROR) << e.what();
        rsp = "Failed to get current region info.";
        rc  = -1;
    }
    return rc;
}

int main_lorawan_region_set(string option, string input_json, string &rsp)
{
    string           region;
    ifstream         json_ifstream;
    ofstream         json_ofstream;
    json             local_json;
    json             remote_json;
    map<string, int> region_m = { { "CN490", 0 }, { "US915", 1 }, { "EU868", 2 } };

    auto rc = -1;
    try {
        json_ifstream.open(LORAWAN_REGION_CONF_DEFAULT);
        json_ifstream >> local_json;
        json_ifstream.close();

        remote_json = json::parse(input_json);
        remote_json["body"]["region"].get_to(region);
        if (region_m.count(region) == 0) {
            LOG(ERROR) << "this region is not exist: " << region << endl;
            rsp = "this region option is not exist: " + region;
            return rc;
        }
        local_json["lorawan_region"] = region;
        LOG(INFO) << "setting region: " << region << endl;
        json_ofstream.open(LORAWAN_REGION_CONF_DEFAULT);
        json_ofstream << local_json.dump(4) << endl;
        json_ofstream.close();
        choose_global_conf_region(region);
        rc = 0;
    } catch (const std::exception &e) {
        LOG(ERROR) << e.what();
        rsp = "Failed to set lorawan region.";
        rc  = -1;
    }
    return rc;
}