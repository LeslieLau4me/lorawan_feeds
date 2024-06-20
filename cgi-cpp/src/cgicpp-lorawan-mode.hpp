/**
 * @file
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 */

#ifndef _CGICPP_LORAWAN_MODE_H_
#define _CGICPP_LORAWAN_MODE_H_
#include <string>
using namespace std;
#define LORAWAN_MODE_CONF_DEFAULT   "/etc/lorawan_workmode/lorawan_workmode.conf"
#define LORAWAN_GLOBAL_CONF_DEFAULT "/etc/lora_pkt_fwd/global_conf.json"
// BaiscStation LNS模式

#define BS_LNS_KEY   "/etc/basicstation/tc.key"
#define BS_LNS_TRUST "/etc/basicstation/tc.trust"
#define BS_LNS_URI   "/etc/basicstation/tc.uri"
#define BS_LNS_CRT   "/etc/basicstation/tc.crt"

/*
    这些端点(文件名不带-boot)中的任何一个成功建立连接后，将复制相应的文件
    到 {tc,cups}-bak.{uri,cert,key,trust} 文件并在 CUPS 事务期间用作后备连接
*/

#define BS_LNS_KEY_BAK   "/etc/basicstation/tc-bak.key"
#define BS_LNS_TRUST_BAK "/etc/basicstation/tc-bak.trust"
#define BS_LNS_URI_BAK   "/etc/basicstation/tc-bak.uri"
#define BS_LNS_CRT_BAK   "/etc/basicstation/tc-bak.crt"
#define BS_LNS_TRUST_DONE  "/etc/basicstation/tc-bak.done"

// BaiscStation CPUS模式

#define BS_CUPS_KEY   "/etc/basicstation/cups.key"
#define BS_CUPS_TRUST "/etc/basicstation/cups.trust"
#define BS_CUPS_URI   "/etc/basicstation/cups.uri"
#define BS_CUPS_CRT   "/etc/basicstation/cups.crt"


#define BS_CUPS_KEY_BAK   "/etc/basicstation/cups-bak.key"
#define BS_CUPS_TRUST_BAK "/etc/basicstation/cups-bak.trust"
#define BS_CUPS_URI_BAK   "/etc/basicstation/cups-bak.uri"
#define BS_CUPS_CRT_BAK   "/etc/basicstation/cups-bak.crt"
#define BS_CUPS_TRUST_DONE "/etc/basicstation/cups-bak.done"

#define BODY              "body"
#define BS                "station"
#define LNS               "LNS"
#define CUPS              "CUPS"
#define BRIDGE            "bridge"
#define WORK_MODE         "work_mode"
#define STATION_WORK_MODE "station_mode"
#define BRIDGE_USE_AUTH   "auth"
#define STATION_AUTH_MODE "auth_mode"
#define STATION_TAG_PATH  "/etc/basicstation"
#define BRIDGE_TAG_PATH   "/etc/lorabridge"

#define TC_TRUST        "/etc/ssl/gw/tc_trust/tc.trust"
#define TC_CLIENT_CRT   "/etc/ssl/gw/tc_crt/tc.crt"
#define TC_CLIENT_KEY   "/etc/ssl/gw/tc_key/tc.key"
#define TC_URI          "/etc/ssl/gw/tc_uri/tc.uri"
#define TC_CLIENT_TOKEN "/etc/ssl/gw/tc_token/tc.key"

#define CUPS_TRUST        "/etc/ssl/gw/cups_boot_trust/cups.trust"
#define CUPS_CLIENT_CRT   "/etc/ssl/gw/cups_boot_crt/cups.crt"
#define CUPS_CLIENT_KEY   "/etc/ssl/gw/cups_boot_key/cups.key"
#define CUPS_URI          "/etc/ssl/gw/cups_boot_uri/cups.uri"
#define CUPS_CLIENT_TOKEN "/etc/ssl/gw/cups_boot_token/cups.key"

#define BRIDGE_CAFILE_PATH   "/etc/ssl/gw/bridge_cafile"
#define BRIDGE_CERTFILE_PATH "/etc/ssl/gw/bridge_certfile"
#define BRIDGE_KEYFILE_PATH  "/etc/ssl/gw/bridge_keyfile"

#define TC_TRUST_NAME "tc.trust"
#define TC_CRT_NAME   "tc.crt"
#define TC_KEY_NAME   "tc.key"

#define CUPS_TRUST_NAME "cups.trust"
#define CUPS_CRT_NAME   "cups.crt"
#define CUPS_KEY_NAME   "cups.key"

#define ETH_NAME_DEFAULT "eth0"
#define MAX_LORA_MAC     6
#define MAX_GATEWAY_ID   16

int  main_lorawan_workmode_get(string option, string input_json, string &rsp);
int  main_lorawan_workmode_set(string option, string input_json, string &rsp);
int  generate_gateway_id_by_mac(char *gw_id);
void remove_files_from_dir_except_input(string dir_path, string input);

#endif