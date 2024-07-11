/**
 * @file
 * @brief  获取/修改网关LoRaWAN 的工作模式及其他附加信息
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 *
 * @details 可以开启、切换，重启当前网关LoRaWAN的工作模式，并进行证书配置等环境操作；
 *          可以查询当前工作模式和LoRaWAN工作环境相关的其他信息，返回json格式的数据内容和运行结果
 */

#include "cgicpp-bridge-topic.hpp"
#include "cgicpp-lorawan-online.hpp"
#include "cgicpp-lorawan-region.hpp"
#include "cgicpp-station.hpp"
#include "easylogging++.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <fstream>
#include <net/if.h>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <sys/dir.h>
#include <sys/ioctl.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;
using json = nlohmann::json;

static string err_file_path;

enum authFileType {
    TYPE_BS_KEY = 0x00,
    TYPE_BS_TRUST,
    TYPE_BS_URI,
    TYPE_BS_CRT,
    TYPE_BS_KEY_BAK,
    TYPE_BS_TRUST_BAK,
    TYPE_BS_URI_BAK,
    TYPE_BS_CRT_BAK,
    TYPE_BS_TRUST_DONE,
};

enum stationMode {
    STATION_LNS_MODE = 0x00,
    STATION_CUPS_MODE,
};

enum authMode {
    AUTH_NONE = 0x00,
    AUTH_TLS_SERVER,
    AUTH_TLS_SERVER_AND_CLIENT,
    AUTH_TLS_SERVER_AND_CLIENT_TOKEN,
};

const char *auth_lns_tb[] = {
    [TYPE_BS_KEY]        = BS_LNS_KEY,
    [TYPE_BS_TRUST]      = BS_LNS_TRUST,
    [TYPE_BS_URI]        = BS_LNS_URI,
    [TYPE_BS_CRT]        = BS_LNS_CRT,
    [TYPE_BS_KEY_BAK]    = BS_LNS_KEY_BAK,
    [TYPE_BS_TRUST_BAK]  = BS_LNS_TRUST_BAK,
    [TYPE_BS_URI_BAK]    = BS_LNS_URI_BAK,
    [TYPE_BS_CRT_BAK]    = BS_LNS_CRT_BAK,
    [TYPE_BS_TRUST_DONE] = BS_LNS_TRUST_DONE,
};

const char *auth_cups_tb[] = {
    [TYPE_BS_KEY]        = BS_CUPS_KEY,
    [TYPE_BS_TRUST]      = BS_CUPS_TRUST,
    [TYPE_BS_URI]        = BS_CUPS_URI,
    [TYPE_BS_CRT]        = BS_CUPS_CRT,
    [TYPE_BS_KEY_BAK]    = BS_CUPS_KEY_BAK,
    [TYPE_BS_TRUST_BAK]  = BS_CUPS_TRUST_BAK,
    [TYPE_BS_URI_BAK]    = BS_CUPS_URI_BAK,
    [TYPE_BS_CRT_BAK]    = BS_CUPS_CRT_BAK,
    [TYPE_BS_TRUST_DONE] = BS_CUPS_TRUST_DONE,
};

static void reload_lorawan_service(void)
{
    system("/etc/lorawan_scripts/lorawan_mode start &");
}

static void shut_down_lorawan_service(void)
{
    system("/etc/lorawan_scripts/lorawan_mode stop &");
}

int get_local_eth_mac(unsigned char *mac_address)
{
    struct ifreq ifr;
    int          sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        LOG(ERROR) << "Failed to socket.";
        return -1;
    }
    strncpy(ifr.ifr_name, ETH_NAME_DEFAULT, IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
        LOG(ERROR) << "Failed to ioctl.";
        close(sock);
        return -1;
    }
    unsigned char *buf = (unsigned char *)ifr.ifr_hwaddr.sa_data;
    memcpy(mac_address, buf, MAX_LORA_MAC);
    close(sock);
    return 0;
}

int generate_gateway_id_by_mac(char *gw_id)
{
    unsigned char mac_buf[MAX_LORA_MAC] = { 0 };
    if (get_local_eth_mac(mac_buf) != 0) {
        return -1;
    }
    char id_buf[MAX_GATEWAY_ID + 1] = { 0 };
    /* clang-format off */
    snprintf(id_buf, MAX_GATEWAY_ID + 1, "%02X%02X%02XFFFE%02X%02X%02X",
        mac_buf[0], mac_buf[1], mac_buf[2], mac_buf[3], mac_buf[4], mac_buf[5]);
    /* clang-format on */
    memcpy(gw_id, id_buf, MAX_GATEWAY_ID + 1);
    return 0;
}

static void get_hardware_region_info(string &region)
{
    json     loacl_json;
    ifstream json_if_region_conf;
    string   current_region;
    /*global_conf.json 出厂默认的频率值，由此判断频段*/
    map<int, string> freq_region = {
        { 904300000, "US915" },
        { 867500000, "EU868" },
        { 471400000, "CN490" },
    };

    json_if_region_conf.open(LORAWAN_REGION_CONF_DEFAULT);
    json_if_region_conf >> loacl_json;
    json_if_region_conf.close();

    loacl_json["lorawan_region"].get_to(current_region);
    // 当一个模块的频段确定下来、完成本地记录，通常不会改变
    for (auto iter : freq_region) {
        if (iter.second == current_region) {
            region = current_region;
            LOG(INFO) << "region had been set: " << current_region;
            break;
        }
    }
}

static int station_adapt_to_spidev_path(void)
{
    DIR           *dir;
    struct dirent *entry;
    const char    *spi_prefix = "spi";
    string         spi_path;
    dir = opendir("/sys/class/spidev");
    if (dir == NULL) {
        LOG(ERROR) << "ERROR: opendir failed.";
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, spi_prefix)) {
            spi_path = string("spi:") + string("/dev/") + string(entry->d_name);
            closedir(dir);
            /*组json 配置station.conf*/
            json js_setting;
            js_setting["body"]["radio_common"]["device"]         = spi_path;
            js_setting["body"]["radio_common"]["clksrc"]         = 0;
            js_setting["body"]["radio_common"]["full_duplex"]    = false;
            js_setting["body"]["radio_common"]["lorawan_public"] = true;
            js_setting["body"]["radio_common"]["pps"]            = false;

            LoRaStation station;
            station.set_remote_string(js_setting.dump(4));
            station.set_local_for_one(BS_RADIO_COMMON_SET);
            return 0;
        }
    }

    closedir(dir);
    return -1;
}

// 删除/etc/basicstation 目录下的所有证书文件
static void station_auth_mode_pre_handle(const int station_mode)
{
    (void)station_mode;
    for (auto p : auth_cups_tb) {
        if (access(p, F_OK) != -1) {
            LOG(INFO) << "[MODE]files remove: " << p;
            remove(p);
        }
    }

    for (auto p : auth_lns_tb) {
        if (access(p, F_OK) != -1) {
            remove(p);
            LOG(INFO) << "[MODE]files remove: " << p;
        }
    }
}

static int station_create_log_file(void)
{
    LoRaStation station;
    string      str_station;
    json        station_conf;
    string      log_path;
    string      folder;
    station.parse_local_for_one(BS_STATION_GET);
    station.get_upload_json(str_station);
    station_conf = json::parse(str_station);
    if (station_conf.contains(STATION_CONF)) {
        log_path = station_conf[STATION_CONF]["log_file"];
        if (log_path.empty()) {
            LOG(ERROR) << "Station log path can't be empty!";
            return -1;
        }
        LOG(INFO) << "station log path:" << log_path;
    }
    /*
        在openwrt 系统无法通过open直接创建一个 目录+文件 格式的绝对路径
        只能先创建目录、并赋予权限之后，才能进一步创建文件
    */

    if (access(log_path.c_str(), F_OK) == -1) {
        auto last_idx = log_path.rfind('/');
        if (last_idx != string::npos) {
            /* 切割文件绝对路径获取文件夹路径 */
            folder = log_path.substr(0, last_idx);
            if (access(folder.c_str(), F_OK) == -1) {
                if (mkdir(folder.c_str(), S_IRUSR | S_IWUSR) < 0) {
                    LOG(ERROR) << "Failed to create dir: " << folder;
                    return -1;
                }
            } else {
                chmod(folder.c_str(), 0600);
                LOG(INFO) << "Target dir does exsit: " << folder;
            }
        } else {
            LOG(ERROR) << "Illegal log dir path: " << folder;
            return -1;
        }
        int ret = open(log_path.c_str(), O_CREAT | O_RDWR, 0660);
        if (ret < 0) {
            LOG(ERROR) << "Failed to create station log file";
            return -1;
        }
    }

    return 0;
}

static int station_auth_mode_none_handle(const int station_mode)
{
    string cmd;
    auto   uri_file = (station_mode == STATION_LNS_MODE) ? TC_URI : CUPS_URI;
    if (access(uri_file, F_OK) != -1) {
        cmd = string("cp ") + string(uri_file) + string(" ") + string(STATION_TAG_PATH);
        system(cmd.c_str());
        LOG(INFO) << "[MODE]current cmd execute: " << cmd;
    } else {
        LOG(ERROR) << "[WARN]Can't not find " << uri_file;
        err_file_path = uri_file;
        return -1;
    }

    return 0;
}

// TLS Server Authentication
static int station_auth_mode_1_handle(const int station_mode)
{
    string cmd;
    // 都是通过cgi-upload 上传到/etc/ssl/gw/ 下的、待拷贝的LNS文件目录
    const char *lns_file_tb[] = { TC_TRUST, TC_URI };
    // 都是通过cgi-upload 上传到/etc/ssl/gw/ 下的、待拷贝的CUPS文件目录
    const char *cup_file_tb[] = { CUPS_TRUST, CUPS_URI };
    auto       &file_table    = (station_mode == STATION_LNS_MODE) ? lns_file_tb : cup_file_tb;
    int         missing_num   = 0;

    // 遍历默认路径upload路径
    for (auto file : file_table) {
        cmd.clear();
        if (access(file, F_OK) != -1) {
            cmd = string("cp ") + string(file) + string(" ") + string(STATION_TAG_PATH);
            system(cmd.c_str());
            LOG(INFO) << "[MODE]current cmd execute:" << cmd;
        } else {
            LOG(ERROR) << "[ERROR]Can't not find " << file;
            missing_num++;
            err_file_path += string(" ") + string(file);
        }
    }
    if (missing_num > 0) {
        return -1;
    }
    return 0;
}

// TLS Server & Client Authentication
static int station_auth_mode_2_handle(const int station_mode)
{
    string cmd;
    // 都是通过cgi-upload 上传到/etc/ssl/gw/ 下的、待拷贝的LNS文件目录
    const char *lns_file_tb[] = { TC_TRUST, TC_CLIENT_CRT, TC_CLIENT_KEY, TC_URI };
    // 都是通过cgi-upload 上传到/etc/ssl/gw/ 下的、待拷贝的CUPS文件目录
    const char *cup_file_tb[] = { CUPS_TRUST, CUPS_CLIENT_CRT, CUPS_CLIENT_KEY, CUPS_URI };
    auto       &file_table    = (station_mode == STATION_LNS_MODE) ? lns_file_tb : cup_file_tb;
    int         missing_num   = 0;

    // 遍历默认路径upload路径
    for (auto file : file_table) {
        cmd.clear();
        if (access(file, F_OK) != -1) {
            cmd = string("cp ") + string(file) + string(" ") + string(STATION_TAG_PATH);
            system(cmd.c_str());
            LOG(INFO) << "[MODE]current cmd execute:" << cmd;
        } else {
            LOG(ERROR) << "[ERROR]Can't not find " << file;
            missing_num++;
            err_file_path += string(" ") + string(file);
        }
    }
    if (missing_num > 0) {
        return -1;
    }
    return 0;
}

// TLS Server & Client Token Authentication
static int station_auth_mode_3_handle(const int station_mode)
{
    string cmd;
    // 都是通过cgi-upload 上传到/etc/ssl/gw/ 下的、待拷贝的LNS文件目录
    const char *lns_file_tb[] = { TC_TRUST, TC_CLIENT_TOKEN, TC_URI };
    // 都是通过cgi-upload 上传到/etc/ssl/gw/ 下的、待拷贝的CUPS文件目录
    const char *cup_file_tb[] = { CUPS_TRUST, CUPS_CLIENT_TOKEN, CUPS_URI };
    auto       &file_table    = (station_mode == STATION_LNS_MODE) ? lns_file_tb : cup_file_tb;
    int         missing_num   = 0;

    // 遍历默认路径upload路径
    for (auto file : file_table) {
        cmd.clear();
        if (access(file, F_OK) != -1) {
            cmd = string("cp ") + string(file) + string(" ") + string(STATION_TAG_PATH);
            system(cmd.c_str());
            LOG(INFO) << "[MODE]current cmd execute:" << cmd;
        } else {
            LOG(ERROR) << "[ERROR]Can't not find " << file;
            missing_num++;
            err_file_path += string(" ") + string(file);
        }
    }
    if (missing_num > 0) {
        return -1;
    }
    return 0;
}

static int station_auth_mode_handle(const int station_mode, const int auth_mode)
{
    station_auth_mode_pre_handle(station_mode);
    // 只需要uri文件
    if (auth_mode == AUTH_NONE) {
        if (station_auth_mode_none_handle(station_mode) != 0) {
            return -1;
        }

        // TLS Server Authentication: 需要uri文件和tust文件
    } else if (auth_mode == AUTH_TLS_SERVER) {
        if (station_auth_mode_1_handle(station_mode) != 0) {
            return -1;
        }

        //TLS Server & Client Authentication: 需要 uri key crt trust
    } else if (auth_mode == AUTH_TLS_SERVER_AND_CLIENT) {
        if (station_auth_mode_2_handle(station_mode) != 0) {
            return -1;
        }
        // TLS Server & Client Token Authentication:  需要 uri Trust(CA) Client_Token
    } else if (auth_mode == AUTH_TLS_SERVER_AND_CLIENT_TOKEN) {
        if (station_auth_mode_3_handle(station_mode) != 0) {
            return -1;
        }
    }

    return 0;
}

// 确保每个证书配置文件夹下至多存在一个配置文件
void remove_files_from_dir_except_input(string dir_path, string input)
{
    if (input.find(dir_path) == string::npos) {
        return;
    }
    string str_dir_path;
    string normal_file;
    string tag_file_path;
    DIR   *pDir = opendir(dir_path.c_str());
    if (pDir == NULL) {
        LOG(ERROR) << "[ERROR]Failed to open this dir: " << dir_path;
        return;
    }

    struct dirent *dir = NULL;
    while ((dir = readdir(pDir))) {
        // 过滤. 和..
        if (string(dir->d_name) == "." || string(dir->d_name) == "..") {
            continue;
        }
        // 普通文件
        if (dir->d_type == DT_REG) {
            normal_file.clear();
            normal_file   = string(dir->d_name);
            tag_file_path = dir_path + "/" + string(dir->d_name);
            if (tag_file_path != input) {
                remove(tag_file_path.c_str());
            }
        }
    }
    closedir(pDir);
}

static int lookup_file_in_directory(string dir_path, string lookup, string &tag_file_path)
{
    string str_dir_path;
    string normal_file;
    DIR   *pDir = opendir(dir_path.c_str());
    if (pDir == NULL) {
        LOG(ERROR) << "[ERROR]Failed to open this dir: " << dir_path;
        return -1;
    }

    struct dirent *dir = NULL;
    while ((dir = readdir(pDir))) {
        // 过滤. 和..
        if (string(dir->d_name) == "." || string(dir->d_name) == "..") {
            continue;
        }
        // 普通文件
        if (dir->d_type == DT_REG) {
            normal_file.clear();
            normal_file = string(dir->d_name);
            if (normal_file.find(lookup) != string::npos) {
                tag_file_path = dir_path + "/" + string(dir->d_name);
                closedir(pDir);
                return 0;
            }
        }
        // 目录文件
        if (dir->d_type == DT_DIR) {
            str_dir_path.clear();
            str_dir_path = dir_path + "/" + string(dir->d_name);
            lookup_file_in_directory(str_dir_path, lookup, tag_file_path);
        }
    }

    closedir(pDir);
    return -1;
}

static int bridge_auth_file_handle(void)
{
    int    missing_num = 0;
    string cmd;
    string file_name;

    map<string, string> path_map = { { BRIDGE_CAFILE_PATH, "ca" },
                                     { BRIDGE_KEYFILE_PATH, "key" },
                                     { BRIDGE_CERTFILE_PATH, "crt" } };

    for (auto iter : path_map) {
        file_name.clear();
        if (lookup_file_in_directory(iter.first, iter.second, file_name) == 0) {
            cmd.clear();
            cmd = "cp " + file_name + " " + BRIDGE_TAG_PATH;
            system(cmd.c_str());
            continue;
        }
        missing_num++;
        err_file_path += " " + string(iter.second);
    }
    if (missing_num > 0) {
        return -1;
    }

    return 0;
}

static void fill_json_for_auth_file(json &upload)
{
    struct file_info {
        char path[64];
        char type[16];
        char name[16];
    };

    struct file_info lns_file_tb[] = {
        { TC_TRUST, "cafile", TC_TRUST_NAME },
        { TC_CLIENT_CRT, "crtfile", TC_CRT_NAME },
        { TC_CLIENT_KEY, "keyfile", TC_KEY_NAME },
    };

    struct file_info cups_file_tb[] = {
        { CUPS_TRUST, "cafile", CUPS_TRUST_NAME },
        { CUPS_CLIENT_CRT, "crtfile", CUPS_CRT_NAME },
        { CUPS_CLIENT_KEY, "keyfile", CUPS_KEY_NAME },
    };

    for (auto p : lns_file_tb) {
        if (access(p.path, F_OK) != -1) {
            upload[BS][LNS][p.type] = p.name;
        } else {
            upload[BS][LNS][p.type] = "";
        }
    }

    /*
     /etc/ssl/gw/tc_trust  目录下默认配备有证书
    */
    if (access(BS_LNS_TRUST, F_OK) != -1) {
        if (access(TC_TRUST, F_OK) == -1) {
            upload[BS][LNS]["cafile"] = TC_TRUST_NAME;
        }
    }

    /*
     /etc/ssl/gw/cups_boot_trust 目录下默认配备的证书
    */
    if (access(BS_CUPS_TRUST, F_OK) != -1) {
        if (access(CUPS_TRUST, F_OK) == -1) {
            upload[BS][CUPS]["cafile"] = CUPS_TRUST_NAME;
        }
    }

    if (access(TC_URI, F_OK) != -1) {
        ifstream ifs(TC_URI);
        string   uri;
        ifs >> uri;
        ifs.close();
        upload[BS][LNS]["urifile"] = uri;
    } else {
        upload[BS][LNS]["urifile"] = "";
    }

    if (access(TC_CLIENT_TOKEN, F_OK) != -1) {
        ifstream ifs(TC_CLIENT_TOKEN);
        // 关闭跳过空格
        ifs >> noskipws;
        istream_iterator<char> it(ifs);
        istream_iterator<char> end;
        string                 token(it, end);
        ifs.close();
        string search_str = "Authorization: ";
        auto   pos        = token.find(search_str);
        if (pos != string::npos) {
            string sub_token         = token.substr(pos + search_str.length());
            upload[BS][LNS]["token"] = sub_token;
        } else {
            upload[BS][LNS]["token"] = "";
        }

    } else {
        upload[BS][LNS]["token"] = "";
    }

    for (auto p : cups_file_tb) {
        if (access(p.path, F_OK) != -1) {
            upload[BS][CUPS][p.type] = p.name;
        } else {
            upload[BS][CUPS][p.type] = "";
        }
    }

    if (access(CUPS_URI, F_OK) != -1) {
        ifstream ifs(CUPS_URI);
        string   uri;
        ifs >> uri;
        ifs.close();
        upload[BS][CUPS]["urifile"] = uri;
    } else {
        upload[BS][CUPS]["urifile"] = "";
    }

    if (access(CUPS_CLIENT_TOKEN, F_OK) != -1) {
        ifstream ifs(CUPS_CLIENT_TOKEN);
        // 关闭跳过空格
        ifs >> noskipws;
        istream_iterator<char> it(ifs);
        istream_iterator<char> end;
        string                 token(it, end);
        ifs.close();
        string search_str = "Authorization: ";
        auto   pos        = token.find(search_str);
        if (pos != string::npos) {
            string sub_token          = token.substr(pos + search_str.length());
            upload[BS][CUPS]["token"] = sub_token;
        } else {
            upload[BS][CUPS]["token"] = "";
        }
    } else {
        upload[BS][CUPS]["token"] = "";
    }
}

int main_lorawan_workmode_get(string option, string input_json, string &rsp)
{
    string              workmode;
    ifstream            json_ifstream;
    json                local_json;
    json                upload_json;
    map<string, string> workmode_m = { { "PKFD", PROC_LORA_PKT_FWD },
                                       { "BAST", PROC_BASICSTATION },
                                       { "BRDG", PROC_LORA_BRIDGE } };

    string station_mode;
    int    auth_mode                      = -1;
    auto   rc                             = -1;
    char   gateway_id[MAX_GATEWAY_ID + 1] = { 0 };
    string reg_freq;
    try {
        json_ifstream.open(LORAWAN_MODE_CONF_DEFAULT);
        json_ifstream >> local_json;
        json_ifstream.close();
        rc = generate_gateway_id_by_mac(gateway_id);
        if (rc != -1) {
            upload_json["gateway_id"] = gateway_id;
        } else {
            upload_json["gateway_id"] = "";
        }
        get_hardware_region_info(reg_freq);
        upload_json["freq"]   = reg_freq;
        local_json[WORK_MODE] = local_json[WORK_MODE].get_to(workmode);
        if (workmode == "BAST") {
            station_mode                = local_json["station_mode"];
            upload_json["station_mode"] = station_mode;
            auth_mode                   = local_json["auth_mode"];
            upload_json["auth_mode"]    = auth_mode;
        }
        upload_json[WORK_MODE] = workmode;
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
        fill_json_for_auth_file(upload_json);
        rsp = upload_json.dump(4);
        rc  = 0;
    } catch (const std::exception &e) {
        LOG(ERROR) << e.what();
        rsp = "Failed to get current work mode info.";
        rc  = -1;
    }
    return rc;
}

int main_lorawan_workmode_set(string option, string input_json, string &rsp)
{
    string           workmode;
    ifstream         json_ifstream;
    ofstream         json_ofstream;
    json             local_json;
    json             remote_json;
    map<string, int> workmode_m = {
        { "PKFD", 0 }, { "BAST", 1 }, { "BRDG", 2 }, { "CLOSE", 3 }, { "RESTART", 4 }
    };
    auto rc = -1;
    try {
        remote_json = json::parse(input_json);
        remote_json[BODY][WORK_MODE].get_to(workmode);
        if (workmode_m.count(workmode) == 0) {
            LOG(ERROR) << "this work mode is not exist: " << workmode << endl;
            rsp = "this work mode option is not exist: " + workmode;
            return rc;
        }
        local_json[WORK_MODE] = workmode;
        if (workmode == "BAST") {
            auto station_mode = -1;
            auto auth_mode    = -1;
            remote_json[BODY][STATION_WORK_MODE].get_to(station_mode);
            local_json[STATION_WORK_MODE] = (station_mode == STATION_LNS_MODE) ? "LNS" : "CUPS";
            remote_json[BODY][STATION_AUTH_MODE].get_to(auth_mode);
            /*0:none 1:TLS Server Authentication 2:TLS Server & Client Authentication
            3:TLS Server & Client Token Authentication:*/
            local_json[STATION_AUTH_MODE] = auth_mode;

            rc = station_auth_mode_handle(station_mode, auth_mode);
            if (rc == -1) {
                rsp = "ERROR: Missing required certification documents with station auth mode:" +
                      to_string(auth_mode) + " err_file_name: " + err_file_path;
                return rc;
            }
            rc = station_adapt_to_spidev_path();
            if (rc < 0) {
                rsp = "Failed to set spidev path correctly.";
                return -1;
            }

            rc = station_create_log_file();
            if (rc < 0) {
                rsp = "Failed to create log file, check your station.conf.";
                return -1;
            }
        } else if (workmode == "BRDG") {

        } else if (workmode == "CLOSE") {
            shut_down_lorawan_service();
            rc = 0;
            return rc;
        } else if (workmode == "RESTART") {
            reload_lorawan_service();
            rc = 0;
            return rc;
        }
        LOG(INFO) << "[MODE]setting work mode: " << workmode << endl;
        json_ofstream.open(LORAWAN_MODE_CONF_DEFAULT);
        json_ofstream << local_json.dump(4) << endl;
        json_ofstream.close();
        reload_lorawan_service();
        rc = 0;
    } catch (const std::exception &e) {
        LOG(ERROR) << e.what();
        rsp = "Failed to set lorawan work mode.";
        rc  = -1;
    }
    return rc;
}