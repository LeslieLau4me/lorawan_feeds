/**
 * @file
 * @brief  获取LTE模块的信息
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 *
 * @details 通过串口写入AT指令获取LTE模块的基本信息，返回json格式的LTE信息和获取结果
 */

#include "cgicpp-lte-serialport.hpp"
#include "easylogging++.h"
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

using namespace std;
using json = nlohmann::json;

class LteSerialPort
{
  private:
    /* data */
    int      fd;
    uint32_t nspeed;
    uint32_t nbits;
    uint8_t  nevent;
    uint8_t  nstop;

    char recv_buf[RECV_BUFF_MAX];
    json upload_json;

    fd_set         readfds;
    struct timeval timeout = { .tv_sec = 0, .tv_usec = 2000 * 100 };

    map<uint32_t, uint8_t> baud_map = {
        { 2400, B2400 }, { 4800, B4800 }, { 9600, B9600 }, { 115200, B115200 }, { 460800, B460800 },
    };

    map<uint8_t, uint8_t> nbit_map = { { 7, CS7 }, { 8, CS8 } };

    enum sim_status {
        NOT_DETECTED = 0x00,
        DETECTED_DIAL_OK,
        DETECTED_DIAL_NOT_OK,
    };
    int sim_status = NOT_DETECTED;

    // 映射关系 <操作类型,AT指令>
    map<enum LteAtcmd, string> at_map = {
        { AT_GET_IMEI, CMD_GET_IMEI },           { AT_GET_ICCID, CMD_GET_ICCID },
        { AT_GET_RSSI, CMD_GET_RSSI },           { AT_GET_SIM_STAT, CMD_GET_SIM_STAT },
        { AT_GET_QNET_STAT, CMD_GET_QNET_STAT }, { AT_SET_ATE0, CMD_SET_ATE0 },
        { AT_CONN_USB, CMD_CONNECT_USB },        { AT_REG_NET, CMD_REGISTER_NETWORK },
        { AT_GET_CPIN, CMD_GET_CPIN },           { AT_SET_SIM_DET, CMD_SET_SIM_DET },
    };

    string err_msg;
    string str_recv;
    bool   sim_networkable = false;

    int  open_lte_dev(void);
    int  set_opt_for_serialport(void);
    int  send_at_get_imei(void);
    int  send_at_get_iccd(void);
    int  send_at_get_rssi(void);
    int  send_at_get_sim_stat(void);
    int  send_at_get_qnet_stat(void);
    int  send_at_cmd_to_serialport(string at_cmd);
    int  lte_serialport_comunication_handler(void);
    void stop_quectel_process(void);
    void restart_quectel_process(void);

  public:
    LteSerialPort();
    LteSerialPort(uint32_t speed, uint32_t bit, uint8_t event, uint8_t stop);
    ~LteSerialPort();
    int  lte_serialport_run(void);
    void get_err_msg(string &msg);
    void get_upload_json(string &upload);
};

void LteSerialPort::stop_quectel_process(void)
{
    system("/etc/init.d/quectel stop");
}

void LteSerialPort::restart_quectel_process(void)
{
    system("/etc/init.d/quectel start &");
}

void LteSerialPort::get_err_msg(string &msg)
{
    msg = this->err_msg;
}

int LteSerialPort::send_at_cmd_to_serialport(string at_cmd)
{
    int ret = -1;
    int len = 0;
    this->str_recv.clear();
    tcflush(fd, TCIOFLUSH);
    LOG(INFO) << "Send AT:" << at_cmd;
    ret = write(this->fd, at_cmd.c_str(), at_cmd.length());
    if (ret < 0) {
        this->err_msg = "Failed to write " + at_cmd;
        LOG(ERROR) << this->err_msg;
        return -1;
    }
    bzero(this->recv_buf, RECV_BUFF_MAX);
    FD_ZERO(&this->readfds);
    FD_SET(this->fd, &this->readfds);
    ret = select(this->fd + 1, &this->readfds, NULL, NULL, &this->timeout);
    if (ret < 0) {
        LOG(ERROR) << "Failed to set.";
        return -1;
    } else if (ret) {
        if (FD_ISSET(this->fd, &this->readfds)) {
            len = read(this->fd, this->recv_buf, RECV_BUFF_MAX);
        }
    } else {
        LOG(ERROR) << "USB timeout.";
        return -1;
    }

    if (len < 0) {
        this->err_msg = "Failed to read.";
        LOG(ERROR) << this->err_msg;
        return -1;
    }

    this->str_recv = string(this->recv_buf);
    auto erase_ch  = { '\r', '\n', 'O', 'K' };
    for (auto ch : erase_ch) {
        this->str_recv.erase(remove(this->str_recv.begin(), this->str_recv.end(), ch),
                             this->str_recv.end());
    }
    LOG(INFO) << "Recv: " << this->str_recv;
    return 0;
}

int LteSerialPort::send_at_get_imei(void)
{
    if (this->send_at_cmd_to_serialport(this->at_map[AT_GET_IMEI]) < 0) {
        return -1;
    }
    try {
        // 截取15位标识码
        string imei = this->str_recv.substr(0, 15);
        LOG(INFO) << "imei: " << imei;
        this->upload_json["imei"] = imei;
    } catch (const std::exception &e) {
        this->err_msg = "Failed to parse imei.";
        LOG(ERROR) << e.what() << '\n';
        return -1;
    }
    return 0;
}

int LteSerialPort::send_at_get_iccd(void)
{
    if (this->send_at_cmd_to_serialport(this->at_map[AT_GET_ICCID]) < 0) {
        return -1;
    }
    return 0;
}

int LteSerialPort::send_at_get_rssi(void)
{
    if (this->send_at_cmd_to_serialport(this->at_map[AT_GET_RSSI]) < 0) {
        return -1;
    }

    int level  = 0;
    int rssi   = 0;
    int signal = -113;

    char        csq[32] = { 0 };
    const char *pstr    = "+CSQ: ";
    auto        pdata   = strstr(this->recv_buf, pstr);
    if (pdata != NULL)
        strncpy(csq, pdata + 6, 6);
    else {
        this->err_msg = "Error msg from tty.";
        return -1;
    }

    /*
    4G (LTE) signal map
    -50 dBm to -65 dBm  Great    Strong signal with very good data speeds -->4
    -65 dBm to -75 dBm	Good	Strong signal with good data speeds -->3
    -75 dBm to -85 dBm	Fair	Fair but useful, fast and reliable data speeds may be attained,
                                but marginal data with drop-outs is possible -->2
    -85 dBm to -95 dBm	Poor	Performance will drop drastically -->1
    <= -95 dBm	No signal	Disconnection  -->0

    */
    map<int, array<int, 2>> level_map = {
        { 4, { -65, -50 } },
        { 3, { -75, -65 } },
        { 2, { -85, -75 } },
        { 1, { -95, -85 } },
    };

    if (strstr(csq, "99,") || strstr(csq, "199,")) {
        LOG(INFO) << "No signal for rssi 99 or 199";
        level = 0;
    } else {
        LOG(INFO) << "csq:" << csq;
        char rssi_str[4];
        bool b_inmap = false;
        bzero(rssi_str, sizeof(rssi_str));
        try {
            if (*(csq + 2) == ',') {
                rssi_str[0] = *(csq + 0);
                rssi_str[1] = *(csq + 1);
            } else if (isdigit(*(csq + 2)) && isdigit(*(csq + 3) == ',')) {
                rssi_str[0] = *(csq + 0);
                rssi_str[1] = *(csq + 1);
                rssi_str[2] = *(csq + 2);
            }
        } catch (const std::exception &e) {
            LOG(ERROR) << e.what() << '\n';
            this->err_msg = "Failed to parse rssi str.";
            return -1;
        }

        rssi   = atoi(rssi_str);
        signal = -113 + (2 * rssi);
        for (auto iter : level_map) {
            if (signal > iter.second[0] && signal <= iter.second[1]) {
                level   = iter.first;
                b_inmap = true;
            }
        }
        if (b_inmap == false) {
            level = 0;
        }
        LOG(INFO) << "level: " << level << " "
                  << "rssi: " << rssi << " "
                  << "signal: " << signal;
    }
    this->upload_json["level"]  = level;
    this->upload_json["rssi"]   = rssi;
    this->upload_json["signal"] = signal;
    return 0;
}

int LteSerialPort::send_at_get_qnet_stat(void)
{
#if 0
    /*
    截取有用信息<type,cid,URC_en,state> 例如<0,0,0,0>
    state:  网络连接状态 0 未连接 1 已连接
    */
    if (this->send_at_cmd_to_serialport(this->at_map[AT_GET_QNET_STAT]) < 0) {
        return -1;
    }
#endif

    /*
    发送AT指令"AT+CREG?"，该指令用于查询网络注册状态。如果返回"+CREG: x,1"或"+CREG: x,5"，则表示已成功注册到网络。
    */
    if (this->send_at_cmd_to_serialport(this->at_map[AT_REG_NET]) < 0) {
        return -1;
    }
#if 0
    string           user_data = this->str_recv.substr(13, 19);
    vector<string>   str_list;
    istringstream    iss(user_data);
    string           token;
    auto             i          = 0;
    array<string, 4> qnet_array = { "type", "cid", "URC_en", "state" };
    while (getline(iss, token, ',')) {
        LOG(INFO) << qnet_array[i] << " is " << token;
        str_list.push_back(token);
        ++i;
    }
    if (str_list[i - 1] == "1") {
        this->sim_networkable = true;
    }
#endif
    if (this->str_recv.find(",1") != string::npos || this->str_recv.find(",5") != string::npos) {
        this->sim_networkable = true;
    }

    return 0;
}

int LteSerialPort::send_at_get_sim_stat(void)
{
    /* 发送ATE<0/1> 决定是否开启回显，这里取消回显 */
    if (this->send_at_cmd_to_serialport(this->at_map[AT_SET_ATE0]) < 0) {
        return -1;
    }
    /* Set (U)SIM card detection pin level as high when (U)SIM card is inserted. */
    if (this->send_at_cmd_to_serialport(this->at_map[AT_SET_SIM_DET]) < 0) {
        return -1;
    }
    if (this->send_at_cmd_to_serialport(this->at_map[AT_GET_CPIN]) < 0) {
        return -1;
    }
    string tmp;
    bool   status = false;
    LOG(INFO) << "recv buffer is." << this->recv_buf;
    if (this->str_recv.find("READY") != string::npos) {
        status = true;
    } else {
        tmp = this->str_recv;
        if (this->send_at_cmd_to_serialport(this->at_map[AT_GET_SIM_STAT]) < 0) {
            return -1;
        }
        if (this->str_recv.find(",1") != string::npos) {
            status = true;
        } else {
            if (tmp.find("REA") != string::npos) {
                status = true;
            }
        }
    }
    if (status != false) {
        LOG(INFO) << "sim status ok.";
        this->send_at_get_qnet_stat();
        if (this->sim_networkable == true) {
            LOG(INFO) << "network status ok.";
            this->sim_status = DETECTED_DIAL_OK;
        } else {
            this->sim_status = DETECTED_DIAL_NOT_OK;
        }
    } else {
        this->sim_status = NOT_DETECTED;
    }

    upload_json["sim_status"] = this->sim_status;

    return 0;
}

LteSerialPort::LteSerialPort() {}

LteSerialPort::LteSerialPort(uint32_t speed, uint32_t bit, uint8_t event, uint8_t stop)
{
    this->nspeed = speed;
    this->nbits  = bit;
    this->nevent = event;
    this->nstop  = stop;
}

void LteSerialPort::get_upload_json(string &upload)
{
    upload = this->upload_json.dump(4);
}

int LteSerialPort::open_lte_dev(void)
{
    string   device_name;
    ifstream json_ifstream;
    json     local_json;

    try {
        json_ifstream.open("/etc/lte_usb/lte_usb.conf");
        json_ifstream >> local_json;
        json_ifstream.close();
        device_name = local_json["AT_ttyUSB"];
    } catch (const std::exception &e) {
        LOG(ERROR) << e.what();
    }

    if (device_name.find("ttyUSB") != string::npos) {
        this->fd = open(device_name.c_str(), O_RDWR | O_NOCTTY);
        if (this->fd < 0) {
            return -1;
        }

        if (this->set_opt_for_serialport() < 0) {
            LOG(WARNING) << "Failed to set attr opt " << device_name;
            close(this->fd);
            return -1;
        }

        LOG(INFO) << device_name << " "
                  << "used for AT(read from conf file) .";
        return 0;
    }

    int    i = 0;
    string buffer;
    bool   b_found     = false;
    char   shell_cmd[] = "ls /dev/BLE* 2>/dev/null | awk '{print $1}' | wc -l";
    FILE  *pipe        = popen(shell_cmd, "r");
    if (!pipe) {
        LOG(ERROR) << "Failed to popen";
        return -1;
    }

    char buf[128];
    while (fgets(buf, sizeof(buf), pipe) != NULL) {
        if ((strlen(buf) > 1) && (buf[strlen(buf) - 1] == '\n'))
            buf[strlen(buf) - 1] = '\0';
    }

    pclose(pipe);

    char *endptr = NULL;
    long  result = strtol(buf, &endptr, 10);
    if (*endptr != '\0' && *endptr != '\n') {
        LOG(ERROR) << "Failed to strtol";
        return -1;
    } else {
        i = result;
    }

    // 跳过BLE的接口
    for (; i <= MAX_TTY_USB_NUM; i++) {
        device_name = string("/dev/ttyUSB") + to_string(i);
        LOG(INFO) << "Try openning " << device_name;
        this->fd = open(device_name.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (this->fd < 0) {
            LOG(WARNING) << "Failed to open " << device_name;
            continue;
        }

        if (this->set_opt_for_serialport() < 0) {
            LOG(WARNING) << "Failed to set attr opt " << device_name;
            close(this->fd);
            continue;
        }

        this->send_at_cmd_to_serialport(string("AT\r\n"));
        buffer.clear();
        buffer = string(this->recv_buf);
        if (buffer.empty()) {
            continue;
        }
        if (buffer.find("OK") != string::npos) {
            b_found = true;
            break;
        }
    }
    if (b_found == false) {
        LOG(ERROR) << "Failed to find and open ttyUSB";
        return -1;
    }
    LOG(INFO) << device_name << " "
              << "used for AT.";

    json     setting_json;
    ofstream json_ofstream;
    setting_json["AT_ttyUSB"] = device_name;
    json_ofstream.open("/etc/lte_usb/lte_usb.conf");
    json_ofstream << setting_json.dump(4) << endl;
    json_ofstream.close();
    return 0;
}

int LteSerialPort::set_opt_for_serialport(void)
{
#if 0
    struct termios tty;
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~CSIZE;
    // 设置数据位
    if (this->nbit_map.count(this->nbits)) {
        tty.c_cflag |= nbit_map[this->nbits];
    } else {
        LOG(ERROR) << "nbit setting error.";
        return -1;
    }
    // 设置检验位
    switch (this->nevent) {
        case 'o':
        case 'O':
            tty.c_cflag |= PARENB;
            tty.c_cflag |= PARODD;
            tty.c_iflag |= (INPCK | ISTRIP);
            break;
        case 'e':
        case 'E':
            tty.c_iflag |= (INPCK | ISTRIP);
            tty.c_cflag |= PARENB;
            tty.c_cflag &= ~PARODD;
            break;
        case 'n':
        case 'N':
            tty.c_cflag &= ~PARENB;
            tty.c_iflag &= ~INPCK;
            break;
        // 默认不校验
        default:
            tty.c_cflag &= ~PARENB;
            tty.c_iflag &= ~INPCK;
            break;
    }

    // 设置波特率
    if (this->baud_map.count(this->nspeed)) {
        cfsetispeed(&tty, baud_map[this->nspeed]);
        cfsetospeed(&tty, baud_map[this->nspeed]);
    } else {
        LOG(ERROR) << "baud setting error.";
        return -1;
    }

    // 设置停止位
    if (this->nstop == 1) {
        tty.c_cflag &= ~CSTOPB;
    } else if (this->nstop == 2) {
        tty.c_cflag |= ~CSTOPB;
    } else {
        LOG(ERROR) << "stop bit setting error.";
        return -1;
    }

    tty.c_cc[VTIME] = 0;
    tty.c_cc[VMIN]  = 0;
    tcflush(this->fd, TCIFLUSH);

    auto ret = tcsetattr(this->fd, TCSANOW, &tty);
    if (ret < 0) {
        LOG(ERROR) << "Failed to set tio attr.";
        return -1;
    }
#endif
    struct termios ios;
    memset(&ios, 0, sizeof(ios));
    tcgetattr(this->fd, &ios);
    cfmakeraw(&ios);
    cfsetispeed(&ios, B115200);
    cfsetospeed(&ios, B115200);
    tcsetattr(this->fd, TCSANOW, &ios);
    tcflush(this->fd, TCIOFLUSH);
    return 0;
}

LteSerialPort::~LteSerialPort()
{
    close(this->fd);
    this->restart_quectel_process();
}

int LteSerialPort::lte_serialport_comunication_handler(void)
{
    if (this->send_at_get_sim_stat() < 0) {
        return -1;
    }

    if (this->send_at_get_imei() < 0) {
        return -1;
    }

    if (this->send_at_get_rssi() < 0) {
        return -1;
    }

    return 0;
}

int LteSerialPort::lte_serialport_run(void)
{
    auto ret = -1;
    this->stop_quectel_process();
    ret = this->open_lte_dev();
    if (ret < 0) {
        return -1;
    }

    ret = this->lte_serialport_comunication_handler();
    if (ret < 0) {
        return -1;
    }

    return 0;
}

int main_lte_get(string option, string input_json, string &rsp)
{
    auto           ret = -1;
    LteSerialPort *lte = new LteSerialPort(115200, 8, 'N', 1);
    ret                = lte->lte_serialport_run();
    if (ret < 0) {
        lte->get_err_msg(rsp);
        delete lte;
        return -1;
    }
    lte->get_upload_json(rsp);
    delete lte;
    return 0;
}
