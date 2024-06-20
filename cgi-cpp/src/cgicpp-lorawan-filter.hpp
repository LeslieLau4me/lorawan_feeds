/**
 * @file
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 */

#ifndef _CGICPP_LORAWAN_FILTER_H_
#define _CGICPP_LORAWAN_FILTER_H_
#include <string>
using namespace std;
#define LORAWAN_FILTER_CONF_DEFAULT "/etc/lorawan_filter/lorawan_filter.conf"

int main_lorawan_filter_set(string option, string input_json, string &rsp);
int main_lorawan_filter_get(string option, string input_json, string &rsp);

#endif