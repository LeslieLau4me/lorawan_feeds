/**
 * @file
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 */

#ifndef _MAIN_HPP_
#define _MAIN_HPP_
#include "cgicpp-bridge-topic.hpp"
#include "cgicpp-lorawan-filter.hpp"
#include "cgicpp-lorawan-mode.hpp"
#include "cgicpp-lorawan-online.hpp"
#include "cgicpp-lorawan-region.hpp"
#include "cgicpp-lte-serialport.hpp"
#include "cgicpp-notification-push.hpp"
#include "cgicpp-station.hpp"
#include "cgicpp-toml.hpp"
#include <iostream>
#include <stdarg.h>
#include <string>
#include <unistd.h>
#define POST_LIMIT    131072
#define SUCCESS_CODE  0
#define FAILURE_CODE  -1
#define LOG_CONF_PATH "/etc/logs/cgicpp_log.conf"
using namespace std;

using cgicpp_cb = int (*)(string option, string input_json, string &rsp);
// typedef int (*cgicpp_cb)(string option, string input_json, string &rsp);

#define TOML_GET   "toml-get"
#define TOML_SET   "toml-set"
#define BS_GET     "bs-get"
#define BS_SET     "bs-set"
#define FILTER_SET "lorawanfilter-set"
#define FILTER_GET "lorawanfilter-get"
#define REGION_SET "lorawanregion-set"
#define REGION_GET "lorawanregion-get"
#define MODE_GET   "workmode-get"
#define MODE_SET   "workmode-set"
#define ONLINE_GET "online-get"

#define BRIDGE_TOPIC_GET "bridgetopic-get"
#define BRIDGE_TOPIC_SET "bridgetopic-set"
#define NOTIF_GET        "notification-get"
#define NOTIF_SET        "notification-set"
#define LTE_GET          "lte-get"

#endif