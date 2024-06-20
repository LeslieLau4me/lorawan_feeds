/**
 * @file
 * @brief  获取网关LoRaWAN的工作模式和进程的运行状态
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 *
 * @details 网关支持Packet Forwarder，Basics Station和MQTT Bridge 三种工作模式，返回json格式的数据内容和运行结果
 */

#include "cgicpp-lorawan-online.hpp"
#include "cgicpp-lorawan-mode.hpp"
#include "easylogging++.h"
#include <dirent.h>
#include <nlohmann/json.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
using namespace std;
using json = nlohmann::json;

pid_t detect_process_state(string procName)
{
    pid_t pid = -1;
    // Open the /proc directory
    DIR *dp = opendir("/proc");
    if (dp != NULL) {
        // Enumerate all entries in directory until process found
        struct dirent *dirp;
        while (pid < 0 && (dirp = readdir(dp))) {
            // Skip non-numeric entries
            pid_t id = atoi(dirp->d_name);
            if (id > 0) {
                // Read contents of virtual /proc/{pid}/cmdline file
                string   cmdPath = string("/proc/") + dirp->d_name + "/cmdline";
                ifstream cmdFile(cmdPath.c_str());
                string   cmdLine;
                getline(cmdFile, cmdLine);
                if (!cmdLine.empty()) {
                    // Keep first cmdline item which contains the program path
                    size_t pos = cmdLine.find('\0');
                    if (pos != string::npos)
                        cmdLine = cmdLine.substr(0, pos);
                    // Keep program name only, removing the path
                    pos = cmdLine.rfind('/');
                    if (pos != string::npos)
                        cmdLine = cmdLine.substr(pos + 1);
                    // Compare against requested process name
                    if (procName == cmdLine)
                        pid = id;
                }
            }
        }
    }

    closedir(dp);
    return pid;
}

int main_lorawan_online_get(string option, string input_json, string &rsp)
{
    int      rc = -1;
    string   workmode;
    ifstream json_ifstream;
    json     local_json;
    json     upload_json;

    map<string, string> workmode_m = { { "PKFD", PROC_LORA_PKT_FWD },
                                       { "BAST", PROC_BASICSTATION },
                                       { "BRDG", PROC_LORA_BRIDGE } };
    try {
        json_ifstream.open(LORAWAN_MODE_CONF_DEFAULT);
        json_ifstream >> local_json;
        json_ifstream.close();

        local_json["work_mode"].get_to(workmode);
        // 出厂时的unknow状态 不在线
        if (workmode_m.count(workmode) == 0) {
            upload_json["online"] = 0;
        } else {
            if (detect_process_state(workmode_m[workmode]) != -1) {
                upload_json["online"] = 1;
            } else {
                upload_json["online"] = 0;
            }
        }
        upload_json["work_mode"] = workmode;
        rsp = upload_json.dump(4);
        rc  = 0;
    } catch (const std::exception &e) {
        LOG(ERROR) << e.what();
        rsp = "Failed to get current online info.";
        rc  = -1;
    }
    return rc;
}