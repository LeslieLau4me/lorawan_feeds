/**
 * @file
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 */

#ifndef _CGICPP_LORAWAN_ONLINE_H_
#define _CGICPP_LORAWAN_ONLINE_H_
using namespace std;
#include <string>

#define PROC_LORA_PKT_FWD "lora_pkt_fwd"
#define PROC_BASICSTATION "station"
#define PROC_LORA_BRIDGE  "lora-gateway-bridge"
int   main_lorawan_online_get(string option, string input_json, string &rsp);
pid_t detect_process_state(string procName);
#endif