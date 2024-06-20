/**
 * @file
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 */

#ifndef _CGICPP_NOTIFICATION_PUSH_H_
#define _CGICPP_NOTIFICATION_PUSH_H_
#include <string>
using namespace std;
#define GW_NOTIF_CONF_DEFAULT "/etc/notification/notification.conf"

int main_notification_push_set(string option, string input_json, string &rsp);
int main_notification_push_get(string option, string input_json, string &rsp);

#endif