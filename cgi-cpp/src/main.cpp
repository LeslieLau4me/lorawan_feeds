/**
 * @file
 * @brief  CGI函数主函数入口
 * @author Copyright (C) Shenzhen Minew Technologies Co., Ltd All rights reserved.
 *
 * @details 识别环境变量和传入命令参数参数执行不同的操作，标准输出http响应报文
 */

#include "main.hpp"
#include "easylogging++.cc"
#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP
using namespace std;

// 主处理程序
class MainHandler
{
  private:
    /* data */

    int    argc;
    char **argv;
    string post;
    string cmd;
    string rsp;
    bool   cmd_line = false;
    string fun_key;
    void   post_read(void);
    void   cmd_parser(void);
    void   set_param(int argc, char **argv);
    void   set_cb(void);
    void   env_handle(void);
    void   main_run(void);
    void   common_content_stdout(uint32_t code, int e, string fmt);
    void   response(uint32_t code, int e, string message, ...);
    void   response(uint32_t code, int e, string data, string message, ...);

    cgicpp_cb cgicpp_handler;
    using entry_type = std::pair<cgicpp_cb, std::string>;

    map<std::string, entry_type> main_map = {
        { TOML_GET, { main_toml_get, "Successfully get bridge toml file." } },
        { TOML_SET, { main_toml_set, "Successfully update bridge toml file." } },
        { BS_GET, { main_station_get, "Successfully get basicstation config." } },
        { BS_SET, { main_station_set, "Successfully update basicstation config." } },
        { FILTER_SET, { main_lorawan_filter_set, "Successfully update filter info." } },
        { FILTER_GET, { main_lorawan_filter_get, "Successfully get filter info." } },
        { REGION_SET, { main_lorawan_region_set, "Successfully update region info." } },
        { REGION_GET, { main_lorawan_region_get, "Successfully get region info." } },
        { MODE_SET, { main_lorawan_workmode_set, "Successfully update LoRaWAN work mode." } },
        { MODE_GET, { main_lorawan_workmode_get, "Successfully get LoRaWAN work mode." } },
        { ONLINE_GET, { main_lorawan_online_get, "Successfully get LoRaWAN online status." } },
        { NOTIF_GET, { main_notification_push_get, "Successfully get push enable state." } },
        { NOTIF_SET, { main_notification_push_set, "Successfully set push enable state." } },
        { LTE_GET, { main_lte_get, "Successfully get lte info." } },
        { BRIDGE_TOPIC_GET, { main_bridge_topic_get, "Successfully get bridge topic." } },
        { BRIDGE_TOPIC_SET, { main_bridge_topic_set, "Successfully update bridge topic." } },

    };

  public:
    MainHandler(/* args */);
    ~MainHandler();

    void cmd_handle(int argc, char **argv);
};

void MainHandler::common_content_stdout(uint32_t code, int e, string fmt)
{
    if (this->cmd_line == false) {
        std::cout << "Status: 200 OK\r" << std::endl;
        std::cout << "Content-Type: application/json\r\n\r" << std::endl;
    }

    std::cout << "{\"header\":{";
    std::cout << "\"code\":" << code;
    if (e != 0) {
        std::cout << ",\"message\":"
                  << "\"" << fmt << "\"";
    } else {
        std::cout << ",\"message\":"
                  << "\"" << fmt << "\"";
    }
}

void MainHandler::response(uint32_t code, int e, string message, ...)
{
    if (message.size() > 0) {
        char    buf[1024];
        va_list args;
        va_start(args, message);
        vsnprintf(buf, sizeof(buf), message.c_str(), args);
        va_end(args);
        this->common_content_stdout(code, e, buf);
        std::cout << "}}" << std::endl;
    }
}

void MainHandler::response(uint32_t code, int e, string data, string message, ...)
{
    if (message.size() > 0) {
        char    buf[1024];
        va_list args;
        va_start(args, message);
        vsnprintf(buf, sizeof(buf), message.c_str(), args);
        va_end(args);
        this->common_content_stdout(code, e, buf);
        std::cout << "}";
    }
    if (data.size() > 0) {
        if ((data.find_last_of("}") != string::npos && data.find_first_of("{") != string::npos) ||
            (data.find_last_of("]") != string::npos && data.find_first_of("[") != string::npos)) {
            std::cout << ",\"body\": " << data;
        } else {
            std::cout << ",\"body\": "
                      << "\"" << data << "\"";
        }
    }
    std::cout << "}" << std::endl;
}

void MainHandler::main_run(void)
{
    if (this->cgicpp_handler(this->cmd, this->post, this->rsp) == SUCCESS_CODE) {
        this->response(200, 0, this->rsp, this->main_map[this->fun_key].second);
    } else {
        if (this->rsp.empty() == true) {
            this->response(500, 1, "cgi-cpp handler failed");
        } else {
            this->response(500, 1, this->rsp);
        }
    }
}

void MainHandler::env_handle(void)
{
    auto env = getenv("HTTP_HOST");
    if (env != NULL) {
        this->post_read();
        LOG(INFO) << "[ENV]env: http host";
    } else {
        this->cmd_line = true;
        if (this->argv[1] != NULL) {
            this->post = this->argv[1];
        }
        LOG(INFO) << "[ENV]env: cmd line";
    }
}

void MainHandler::set_cb(void)
{
    vector<string> str_list;
    istringstream  iss(this->cmd);
    string         token;
    auto           i = 0;
    while (getline(iss, token, '-')) {
        str_list.push_back(token);
        ++i;
    }
    /*拆分 cgicpp-*-*-* 第二个*是操作对象，最后一个是*操作类型 */
    this->fun_key        = str_list[1] + string("-") + str_list[i - 1];
    if (this->main_map.find(this->fun_key) != this->main_map.end()) {
        this->cgicpp_handler = this->main_map[this->fun_key].first;
    } else {
        LOG(ERROR) << "Invalid CGI command.";
        exit(-1);
    }

}

MainHandler::MainHandler(/* args */)
{
    el::Configurations conf(LOG_CONF_PATH);
    el::Loggers::reconfigureAllLoggers(conf);
    LOG(INFO) << "*****cgicpp start logging  *****";
}

MainHandler::~MainHandler() {}

void MainHandler::cmd_parser(void)
{
    this->cmd = this->argv[0];
    vector<string> str_list;
    istringstream  iss(this->cmd);
    string         token;
    auto           i = 0;
    while (getline(iss, token, '/')) {
        str_list.push_back(token);
        ++i;
    }
    this->cmd = str_list[i - 1];
    LOG(INFO) << "[CMD]cmd after parsing : " << this->cmd;
}

void MainHandler::post_read(void)
{
    char   *p;
    ssize_t rlen = 0;
    auto    var  = getenv("CONTENT_LENGTH");
    if (var == NULL) {
        return;
    }
    auto content_length = strtol(var, &p, 10);
    if (p == var || content_length <= 0 || content_length >= POST_LIMIT) {
        return;
    }

    char *post_buf = new char[content_length + 1];
    memset(post_buf, 0, content_length + 1);
    if (post_buf == NULL) {
        return;
    }
    for (int len = 0; len < content_length; len += rlen) {
        rlen = read(0, post_buf + len, content_length - len);
        if (rlen <= 0) {
            break;
        }
    }
    this->post = post_buf;
    delete post_buf;
}

void MainHandler::set_param(int argc, char **argv)
{
    this->argc = argc;
    this->argv = argv;
}

void MainHandler::cmd_handle(int argc, char **argv)
{
    this->set_param(argc, argv);
    this->cmd_parser();
    this->set_cb();
    this->env_handle();
    this->main_run();
}

int main(int argc, char *argv[])
{
    MainHandler cgi_main;
    cgi_main.cmd_handle(argc, argv);
    return 0;
}
