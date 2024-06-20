/**
 * @file
 * @brief  解析主函数操作命令的父类
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 *
 * @details 通过重写父类的虚函数实现各自的修改/读取操作
 */

#include "cgicpp-parser-base.hpp"
#include "easylogging++.h"
#include <fstream>
#include <nlohmann/json.hpp>

using namespace std;
CgiParserBase::~CgiParserBase() {}

int main_parser_get(CgiParserBase *obj, string option, string input_json, string &rsp)
{
    int rc = -1;
    try {
        auto idx = option.find("all");
        if (idx != string::npos) {
            obj->parse_local_for_each();
        } else {
            obj->parse_local_for_one(option);
        }
        obj->get_upload_json(rsp);
        rc = 0;
    } catch (const std::exception &e) {
        rsp = "Failed to get" + obj->get_parser_name() +
              "config information, please check your local json file.";
        LOG(ERROR) << e.what();
    }
    return rc;
}

int main_parser_set(CgiParserBase *obj, string option, string input_json, string &rsp)
{
    int rc = -1;
    try {
        obj->set_remote_string(input_json);
        auto idx = option.find("all");
        if (idx != string::npos) {
            obj->set_local_for_each();
        } else {
            obj->set_local_for_one(option);
        }
        rc = 0;
    } catch (const std::exception &e) {
        rsp = "Failed to update " + obj->get_parser_name() +
              " config, please check your json format or option value.";
        LOG(ERROR) << e.what();
    }
    return rc;
}
