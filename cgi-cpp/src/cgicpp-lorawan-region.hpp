/**
 * @file
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 */

#ifndef _CGICPP_LORAWAN_REGION_H_
#define _CGICPP_LORAWAN_REGION_H_
#include <string>
using namespace std;
#define LORAWAN_REGION_CONF_DEFAULT "/etc/lorawan_region/lorawan_region.conf"
#define GLOBAL_CONF_SX1250_COMMON   "/etc/lora_pkt_fwd/global_conf.json.sx1250"
#define GLOBAL_CONF_SX1250          "/etc/lora_pkt_fwd/global_conf.json"

int main_lorawan_region_get(string option, string input_json, string &rsp);
int main_lorawan_region_set(string option, string input_json, string &rsp);

#endif