#include <arpa/inet.h>
#include <errno.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <ifaddrs.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define ETHENET_NAME_DEFAULT "eth0"
#define MAX_BUF_SIZE         4096
#define ARRAY_SIZE(a)        (sizeof(a) / sizeof((a)[0]))

enum rgb_status {
    GREEN_LIGHT = 0x00,
    RED_LIGHT,
    LIGHT_OFF,
};

struct event_base    *ev_base;
static const char    *ping_list[] = { "8.8.8.8", "8.8.4.4", "1.1.1.1", "1.0.0.1", "www.baidu.com" };
static struct timeval ping_interval = { .tv_sec = 5, .tv_usec = 0 };
time_t                start_time;
static int            light_status  = LIGHT_OFF;
bool                  network_ok    = false;
pthread_mutex_t       console_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_t       rgb_tid;
static bool     b_rgb_breath = false;
pthread_mutex_t rgb_mutex;
pthread_cond_t  rgb_cond;

static char gateway_ip[INET_ADDRSTRLEN] = { 0 };

static int get_gateway_ip_addr(void)
{
    struct ifaddrs *ifaddr, *ifa;
    char            ip[INET_ADDRSTRLEN];

    if (getifaddrs(&ifaddr) == -1) {
        perror("Failed to get interface addresses");
        return -1;
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }
        struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
        inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN);
        printf("Interface: %s, IP: %s\n", ifa->ifa_name, ip);
        if (strstr(ifa->ifa_name, "eth")) {
            strncpy(gateway_ip, ip, INET_ADDRSTRLEN);
            char *lastDot = strrchr(gateway_ip, '.');
            if (lastDot != NULL) {
                // 将点后面的数字改为 1
                *(++lastDot) = '1';
                if (*(++lastDot) != ' ') {
                    *(lastDot) = ' ';
                }
                if (*(++lastDot) != ' ') {
                    *(lastDot) = ' ';
                }
                gateway_ip[INET_ADDRSTRLEN - 1] = '\0';
                gateway_ip[INET_ADDRSTRLEN - 2] = '\0';
            }
            printf("gateway_ip: %s\n", gateway_ip);
            break;
        }
    }

    freeifaddrs(ifaddr);
    return 0;
}

static void red_rgb_turn_on(void)
{
    system("sh /usr/bin/led_control.sh red");
}

static void green_rgb_turn_on(void)
{
    system("sh /usr/bin/led_control.sh green");
}

static void rgb_turn_off(void)
{
    system("sh /usr/bin/led_control.sh off");
}

void netlink_rgb_breath_set_flag(bool flag)
{
    pthread_mutex_lock(&rgb_mutex);
    if (flag == b_rgb_breath) {
        goto end;
    }

    if (b_rgb_breath == false && flag == true) {
        // 唤醒子线程
        pthread_cond_signal(&rgb_cond);
        network_ok   = true;
        light_status = GREEN_LIGHT;
    } else if (b_rgb_breath == true && flag == false) {
        network_ok   = false;
        light_status = RED_LIGHT;
    }
    b_rgb_breath = flag;

end:
    pthread_mutex_unlock(&rgb_mutex);
}

void netlink_init_sync_objects(void)
{
    pthread_mutex_init(&rgb_mutex, NULL);
    pthread_cond_init(&rgb_cond, NULL);
}

void netlink_destroy_sync_objects(void)
{
    // 销毁互斥锁和条件变量
    pthread_mutex_destroy(&rgb_mutex);
    pthread_cond_destroy(&rgb_cond);
}

void *netlink_rgb_thread(void *arg)
{
    while (true) {
        // 上锁
        pthread_mutex_lock(&rgb_mutex);
        // 如果没有收到数据，则等待条件变量被唤醒
        while (!b_rgb_breath) { pthread_cond_wait(&rgb_cond, &rgb_mutex); }
        pthread_mutex_unlock(&rgb_mutex);
        green_rgb_turn_on();
        usleep(1000 * 1000 * 1.8);
        rgb_turn_off();
    }

    return NULL;
}

static void ping_network_status(void)
{
    int  status;
    char ping_cmd[256] = { 0 };

    sprintf(ping_cmd, "ping -c 1 %s > /dev/null 2>&1", ping_list[0]);
    status = system(ping_cmd);
    if (status == 0) {
        printf("INFO: Network link is up and available.\n");
        netlink_rgb_breath_set_flag(true);
        return;
    }

    memset(ping_cmd, 0, sizeof(ping_cmd));
    sprintf(ping_cmd, "ping -c 1 %s > /dev/null 2>&1", gateway_ip);
    status = system(ping_cmd);
    if (status == 0) {
        printf("INFO: Network link is up and available.\n");
        netlink_rgb_breath_set_flag(true);
        return;
    }

    printf("ERROR: Network exception\n");
    netlink_rgb_breath_set_flag(false);
    red_rgb_turn_on();
    return;
}

static void timer_cb(evutil_socket_t fd, short what, void *arg)
{
    pthread_mutex_lock(&console_mutex);
    ping_network_status();
    pthread_mutex_unlock(&console_mutex);
    // 网络ok后逐渐进入深睡眠，不再频繁消耗网络资源
    if (network_ok) {
        time_t current_time = time(NULL);
        int    elapsed_time = (int)difftime(current_time, start_time);
        if (elapsed_time / 60) {
            start_time = current_time;
            // 5分钟重置一次
            if (ping_interval.tv_sec == 30) {
                ping_interval.tv_sec = 5;
            } else {
                ping_interval.tv_sec += 5;
            }
        }
    } else { // 重新恢复5s定时
        ping_interval.tv_sec = 5;
    }
    printf("INFO: Current interval: %ld \n", ping_interval.tv_sec);
    struct event *ev = event_new(ev_base, -1, EV_TIMEOUT, timer_cb, (void *)ev_base);
    if (!ev) {
        fprintf(stderr, "Failed to create event\n");
    }
    if (event_add(ev, &ping_interval) < 0) {
        fprintf(stderr, "Failed to add event\n");
    }
}

static void signal_cb(evutil_socket_t sig, short events, void *user_data)
{
    printf("INFO: sig:[%d], program will exit...\n", sig);
    event_base_loopexit(ev_base, NULL);
    pthread_cancel(rgb_tid);
}

void read_cb(struct bufferevent *bev, void *ctx)
{
    char             buffer[MAX_BUF_SIZE];
    ssize_t          len;
    struct nlmsghdr *nlh;
    struct rtmsg    *rtm;
    pthread_mutex_lock(&console_mutex);
    while ((len = bufferevent_read(bev, buffer, sizeof(buffer))) > 0) {
        for (nlh = (struct nlmsghdr *)buffer; NLMSG_OK(nlh, len); nlh = NLMSG_NEXT(nlh, len)) {
            if (nlh->nlmsg_type == NLMSG_DONE) {
                return;
            }
            if (nlh->nlmsg_type == NLMSG_ERROR) {
                fprintf(stderr, "Netlink error\n");
                return;
            }
            if (nlh->nlmsg_type == RTM_NEWROUTE || nlh->nlmsg_type == RTM_DELROUTE) {
                rtm = (struct rtmsg *)NLMSG_DATA(nlh);
                if (rtm->rtm_family == AF_INET || rtm->rtm_family == AF_INET6) {
                    printf("Route updated\n");
                    ping_network_status();
                }
            }
        }
    }
    pthread_mutex_unlock(&console_mutex);
}

int main(void)
{
    int ret;
    ping_network_status();
    ret = get_gateway_ip_addr();
    if (ret < 0) {
        printf("WARN: Failed to get lan gateway ip.\n");
    }
    netlink_init_sync_objects();

    pthread_create(&rgb_tid, NULL, netlink_rgb_thread, NULL);
    ping_network_status();
    struct sockaddr_nl sa;
    memset(&sa, 0, sizeof(sa));

    // 创建netlink套接字
    int netlink_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (netlink_fd == -1) {
        printf("ERROR: Faied to new a socket.\n");
        return -1;
    }

    // 设置套接字参数
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = RTMGRP_IPV4_ROUTE | RTMGRP_IPV6_ROUTE; // 监听网络设备变化

    // 绑定套接字
    if (bind(netlink_fd, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
        printf("ERROR: Faied to bind.\n");
        close(netlink_fd);
        return -1;
    }

    // 创建事件基础结构体
    ev_base = event_base_new();
    if (!ev_base) {
        fprintf(stderr, "Failed to create event base\n");
        close(netlink_fd);
        return -1;
    }

    // 创建netlink事件
    struct bufferevent *nl_bev = bufferevent_socket_new(ev_base, netlink_fd, BEV_OPT_CLOSE_ON_FREE);
    if (!nl_bev) {
        fprintf(stderr, "Failed to create event\n");
        close(netlink_fd);
        event_base_free(ev_base);
        return -1;
    }

    // 设置读事件回调函数
    bufferevent_setcb(nl_bev, read_cb, NULL, NULL, NULL);

    // 启用读事件
    bufferevent_enable(nl_bev, EV_READ);
    struct event *signal_event = evsignal_new(ev_base, SIGINT, signal_cb, NULL);
    if (!signal_event || event_add(signal_event, NULL) < 0) {
        fprintf(stderr, "Could not create/add a signal event!\n");
        return -1;
    }
    // EV_TIMEOUT 改成 EV_PERSIST可一直定时
    struct event *timer_ev = event_new(ev_base, -1, EV_TIMEOUT, timer_cb, NULL);
    if (!timer_ev || event_add(timer_ev, &ping_interval) < 0) {
        fprintf(stderr, "Could not create/add a timer event!\n");
        return -1;
    }

    start_time = time(NULL);
    // 进入事件循环
    event_base_dispatch(ev_base);
    pthread_join(rgb_tid, NULL);
    // 清理资源
    close(netlink_fd);
    // event_free(nl_event);
    bufferevent_free(nl_bev);
    event_base_free(ev_base);
    rgb_turn_off();
    pthread_mutex_destroy(&console_mutex);
    netlink_destroy_sync_objects();
    printf("INFO: program exit...\n");
    return 0;
}
