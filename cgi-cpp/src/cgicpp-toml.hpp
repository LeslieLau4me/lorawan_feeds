/**
 * @file
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 */

#ifndef _CGICPP_TOML_HPP_
#define _CGICPP_TOML_HPP_
#include <bits/stdc++.h> // system() func support
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <nlohmann/json.hpp>
#include <toml.hpp>

using namespace std;

// #define DEBUG_TOML
#ifdef DEBUG_TOML
    #define TOML_COUT cout
#else
    #define TOML_COUT 0 && cout
#endif

// #define DEBUG_JSON
#ifdef DEBUG_JSON
    #define JSON_COUT cout
#else
    #define JSON_COUT 0 && cout
#endif

#define TMP_FILE_NAME       "/tmp/tmp.toml"
#define BRIDGE_CONF_DEFAULT "/etc/lorabridge/lorabridge.toml"

#define GET_GENERAL      "cgicpp-toml-general-get"
#define GET_FILTERS      "cgicpp-toml-filters-get"
#define GET_BACKEND      "cgicpp-toml-backend-get"
#define GET_INTERGRATION "cgicpp-toml-integration-get"
#define GET_METRICS      "cgicpp-toml-metrics-get"
#define GET_META_DATA    "cgicpp-toml-meta-data-get"
#define GET_COMMAND      "cgicpp-toml-commands-get"
#define GET_ALL          "cgicpp-toml-all-get"

#define SET_GENERAL      "cgicpp-toml-general-set"
#define SET_FILTERS      "cgicpp-toml-filters-set"
#define SET_BACKEND      "cgicpp-toml-backend-set"
#define SET_INTERGRATION "cgicpp-toml-integration-set"
#define SET_METRICS      "cgicpp-toml-metrics-set"
#define SET_META_DATA    "cgicpp-toml-meta-data-set"
#define SET_COMMAND      "cgicpp-toml-commands-set"
#define SET_ALL          "cgicpp-toml-all-set"

#define BASIC_STATION "basic_station"
#define SEMTECH_UDP   "semtech_udp"

#define MQTT       "mqtt"
#define GENERIC    "generic"
#define GCP_CLOUND "gcp_cloud_iot_core"
#define AZURE_IOT  "azure_iot_hub"

#define HEADER "header"
#define BODY   "body"

int main_toml_get(string option, string input_json, string &rsp);
int main_toml_set(string option, string input_json, string &rsp);

#endif