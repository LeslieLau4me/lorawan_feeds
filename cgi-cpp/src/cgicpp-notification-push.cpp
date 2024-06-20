/**
 * @file
 * @brief  获取/修改网关当前的固件推送状态
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 *
 * @details 推送状态为开时，打开web页面会有弹框提示更新，返回json格式的数据内容和运行结果
 */

#include "cgicpp-notification-push.hpp"
#include "easylogging++.h"
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

int main_notification_push_get(string option, string input_json, string &rsp)
{
    bool     push_enable;
    ifstream json_ifstream;
    json     local_json;
    json     upload_json;

    auto rc = -1;
    try {
        json_ifstream.open(GW_NOTIF_CONF_DEFAULT);
        json_ifstream >> local_json;
        json_ifstream.close();

        local_json["push_enable"].get_to(push_enable);
        upload_json["push_enable"] = push_enable;
        rsp                        = upload_json.dump(4);
        rc                         = 0;
    } catch (const std::exception &e) {
        LOG(ERROR) << e.what();
        rsp = "Failed to get push enable state.";
        rc  = -1;
    }
    return rc;
}

int main_notification_push_set(string option, string input_json, string &rsp)
{
    bool     push_enable;
    ifstream json_ifstream;
    ofstream json_ofstream;
    json     local_json;
    json     remote_json;

    auto rc = -1;
    try {
        json_ifstream.open(GW_NOTIF_CONF_DEFAULT);
        json_ifstream >> local_json;
        json_ifstream.close();

        remote_json = json::parse(input_json);
        remote_json["body"]["push_enable"].get_to(push_enable);
        local_json["push_enable"] = push_enable;

        LOG(INFO) << "push enable: " << push_enable << endl;
        json_ofstream.open(GW_NOTIF_CONF_DEFAULT);
        json_ofstream << local_json.dump(4) << endl;
        json_ofstream.close();
        rc = 0;
    } catch (const std::exception &e) {
        LOG(ERROR) << e.what();
        rsp = "Failed to set push enable state.";
        rc  = -1;
    }
    return rc;
}