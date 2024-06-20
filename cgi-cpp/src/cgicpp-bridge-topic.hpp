/**
 * @file
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 */

#ifndef _CGICPP_BRIDGE_TOPIC_H_
#define _CGICPP_BRIDGE_TOPIC_H_

#include <string>
using namespace std;
#define BRIDGE_TOPIC_CONF_DEFAULT "/etc/lorabridge/lorabridge_topic.conf"

int main_bridge_topic_get(string option, string input_json, string &rsp);
int main_bridge_topic_set(string option, string input_json, string &rsp);

#endif