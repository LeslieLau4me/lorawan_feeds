/**
 * @file
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 */

#ifndef _CGICPP_LTE_SERIALPORT_H_
#define _CGICPP_LTE_SERIALPORT_H_
#include <string>
using namespace std;
#define LTE_DEV_DEFAULT "/dev/ttyUSB4"
#define RECV_BUFF_MAX   64
#define MAX_TTY_USB_NUM 7

enum LteAtcmd {
    AT_GET_IMEI = 0x00, /*用于返回 ME 的国际移动设备识别码（IMEI 号）*/
    AT_GET_ICCID,       /*该命令用于查询(U)SIM 卡的集成电路卡识别码（ICCID）*/
    AT_GET_RSSI, /*该命令用于查询当前服务小区接收信号强度<rssi>和信道误码率<ber>*/
    AT_GET_SIM_STAT, /*该命令用于查询/启用/禁用(U)SIM 卡的插拔状态上报功能。*/
    AT_SET_SIM_DET,  /*该命令用于设置(U)SIM 卡的插入时检测管脚电平。*/
    AT_GET_QNET_STAT,
    AT_SET_ATE0,
    AT_CONN_USB,
    AT_REG_NET,
    AT_GET_CPIN
};

#define CMD_SET_ATE0      "ATE0\r\n"
#define CMD_GET_IMEI      "AT+GSN\r\n"         /* 862821063001792 */
#define CMD_GET_ICCID     "AT+QCCID\r\n"       /* +QCCID: 898604411920C0124083 */
#define CMD_GET_RSSI      "AT+CSQ\r\n"         /* +CSQ: 27,99 */
#define CMD_GET_SIM_STAT  "AT+QSIMSTAT?\r\n"   /*  OK +QSIMSTAT: 1,0 */
#define CMD_GET_QNET_STAT "AT+QNETDEVCTL?\r\n" /*拨号联网*/
#define CMD_CONNECT_USB   "AT+QNETDEVCTL\r\n"
#define CMD_SET_SIM_DET   "AT+QSIMDET=1,1\r\n"

#define CMD_REGISTER_NETWORK "AT+CREG?\r\n"
#define CMD_GET_CPIN         "AT+CPIN?\r\n"

int main_lte_get(string option, string input_json, string &rsp);

#endif