#include "net/tcp_client.h"

#include "log/log.h"
#include "net/json_protocol.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define TCP_CLIENT_MAX_JSON_LEN (64 * 1024)
#define TCP_CLIENT_RECONNECT_DELAY_SEC 3
#define TCP_CLIENT_HEARTBEAT_INTERVAL_SEC 10  

static pthread_t g_client_thread;
static pthread_mutex_t g_socket_mutex = PTHREAD_MUTEX_INITIALIZER;

static int g_thread_started = 0;
static int g_running = 0;
static int g_socket_fd = -1;
static char g_server_ip[64];
static int g_server_port = 0;

/*
 * @brief 关闭当前 socket，并把全局连接状态设置为未连接
 */
static void close_current_socket(void)
{
    pthread_mutex_lock(&g_socket_mutex);

    if (g_socket_fd >= 0) {
        close(g_socket_fd);
        g_socket_fd = -1;
    }

    pthread_mutex_unlock(&g_socket_mutex);
}

/*
 * @brief 循环发送，直到指定长度的数据全部发送完成
 */
static int send_all_raw(int fd, const void *buf, int len)
{
    const char *p = (const char *)buf;
    int sent_total = 0;

    while (sent_total < len) {
        int sent;

#ifdef MSG_NOSIGNAL
        sent = (int)send(fd, p + sent_total, len - sent_total, MSG_NOSIGNAL);
#else
        sent = (int)send(fd, p + sent_total, len - sent_total, 0);
#endif

        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        if (sent == 0) {
            return -1;
        }

        sent_total += sent;
    }

    return 0;
}

/*
 * @brief 循环接收，直到指定长度的数据全部接收完成
 */
static int recv_all_raw(int fd, void *buf, int len)
{
    char *p = (char *)buf;
    int recv_total = 0;

    while (recv_total < len) {
        int n = (int)recv(fd, p + recv_total, len - recv_total, 0);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        if (n == 0) {
            return -1;
        }

        recv_total += n;
    }

    return 0;
}

/*
 * @brief 连接 Qt 后台 TCP Server
 */
static int connect_to_server(const char *server_ip, int server_port)
{
    int fd;
    struct sockaddr_in addr;
    struct timeval timeout;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        LOG_WARN("tcp client create socket failed");
        return -1;
    }

    //设置超时时间  
    timeout.tv_sec = 5;     //5s
    timeout.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)); //设置接收超时
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)); //设置发送超时

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)server_port);

    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) != 1) {
        LOG_WARN("tcp client invalid server ip: %s", server_ip);
        close(fd);
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        LOG_WARN("tcp client connect failed, server=%s:%d", server_ip, server_port);
        close(fd);
        return -1;
    }

    LOG_INFO("tcp client connected, server=%s:%d", server_ip, server_port);
    return fd;
}

/*
 * @brief 接收一条完整 JSON 消息
 *
 * 消息格式：4 字节网络字节序长度头 + JSON 正文。
 */
static char *recv_json_packet(int fd)
{
    uint32_t net_len;   //网络字节序长度
    uint32_t json_len;  //本地字节序真正长度数字
    char *json_text;

    if (recv_all_raw(fd, &net_len, sizeof(net_len)) != 0) {
        return NULL;
    }

    json_len = ntohl(net_len);
    if (json_len == 0 || json_len > TCP_CLIENT_MAX_JSON_LEN) {
        LOG_WARN("tcp client invalid json length: %u", json_len);
        return NULL;
    }

    json_text = (char *)malloc(json_len + 1);
    if (json_text == NULL) {
        LOG_WARN("tcp client malloc failed, json_len=%u", json_len);
        return NULL;
    }

    if (recv_all_raw(fd, json_text, (int)json_len) != 0) {
        free(json_text);
        return NULL;
    }

    json_text[json_len] = '\0';
    return json_text;
}

/*
 * @brief 等待 socket 可读，用超时避免线程无法及时退出
 */
static int wait_socket_readable(int fd, int timeout_sec)
{
    fd_set read_set;
    struct timeval timeout;
    int ret;

    FD_ZERO(&read_set);
    FD_SET(fd, &read_set);

    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;

    ret = select(fd + 1, &read_set, NULL, NULL, &timeout);
    if (ret > 0 && FD_ISSET(fd, &read_set)) {
        return 1;
    }

    if (ret < 0 && errno != EINTR) {
        return -1;
    }

    return 0;
}

/*
 * @brief 处理后台发来的一条请求 JSON
 */
static int handle_backend_json(const char *request_json)
{
    char *response_json;
    int ret;

    response_json = json_protocol_handle_request(request_json);
    if (response_json == NULL) {
        LOG_WARN("tcp client handle request failed, no response");
        return -1;
    }

    ret = tcp_client_send_json(response_json);
    json_protocol_free(response_json);

    return ret;
}

/*
 * @brief 发送设备心跳
 */
static int send_heartbeat(void)
{
    char *heartbeat_json;
    int ret;

    heartbeat_json = json_protocol_build_heartbeat();
    if (heartbeat_json == NULL) {
        return -1;
    }

    ret = tcp_client_send_json(heartbeat_json);
    json_protocol_free(heartbeat_json);

    return ret;
}

/*
 * @brief TCP 客户端后台线程
 *
 * 线程负责自动连接、断线重连、心跳发送和后台命令接收。
 */
static void *tcp_client_thread_func(void *arg)
{
    (void)arg;

    //外层while负责自动连接、断线重连
    while (g_running) {
        int fd = connect_to_server(g_server_ip, g_server_port);
        time_t last_heartbeat = 0;

        if (fd < 0) {
            sleep(TCP_CLIENT_RECONNECT_DELAY_SEC);
            continue;
        }

        pthread_mutex_lock(&g_socket_mutex);
        g_socket_fd = fd;
        pthread_mutex_unlock(&g_socket_mutex);

        //发送第一次心跳
        send_heartbeat();
        last_heartbeat = time(NULL);

        //内层while负责心跳发送和后台命令接收
        while (g_running) {
            int wait_ret;
            time_t now = time(NULL);

            //每10秒发送一次心跳包
            if (now - last_heartbeat >= TCP_CLIENT_HEARTBEAT_INTERVAL_SEC) {
                if (send_heartbeat() != 0) {
                    LOG_WARN("tcp client send heartbeat failed");
                    break;  //发送心跳失败，退出内层while
                }
                last_heartbeat = now;
            }

            //等待后台发数据，最多等1s
            wait_ret = wait_socket_readable(fd, 1);
            if (wait_ret < 0) {
                LOG_WARN("tcp client select failed");
                break;
            }

            //收到后台请求后处理
            if (wait_ret > 0) {
                char *request_json = recv_json_packet(fd);

                if (request_json == NULL) {
                    LOG_WARN("tcp client recv json failed");
                    break;
                }

                if (handle_backend_json(request_json) != 0) {
                    LOG_WARN("tcp client send response failed");
                    free(request_json);
                    break;
                }

                free(request_json);
            }
        }

        close_current_socket();

        if (g_running) {
            LOG_INFO("tcp client disconnected, reconnect later");
            sleep(TCP_CLIENT_RECONNECT_DELAY_SEC);
        }
    }

    return NULL;
}

/*
 * @brief 启动 TCP 客户端
 */
int tcp_client_start(const char *server_ip, int server_port)
{
    if (server_ip == NULL || server_port <= 0 || server_port > 65535) {
        LOG_WARN("tcp client start invalid param");
        return -1;
    }

    if (g_thread_started) {
        LOG_INFO("tcp client already started");
        return 0;
    }

    snprintf(g_server_ip, sizeof(g_server_ip), "%s", server_ip);
    g_server_port = server_port;
    g_running = 1;

    if (pthread_create(&g_client_thread, NULL, tcp_client_thread_func, NULL) != 0) {
        g_running = 0;
        LOG_WARN("tcp client create thread failed");
        return -1;
    }

    g_thread_started = 1;
    LOG_INFO("tcp client started, server=%s:%d", g_server_ip, g_server_port);

    return 0;
}

void tcp_client_stop(void)
{
    if (!g_thread_started) {
        return;
    }

    g_running = 0;
    close_current_socket();
    pthread_join(g_client_thread, NULL);
    g_thread_started = 0;

    LOG_INFO("tcp client stopped");
}

/*
 * @brief 发送 JSON 字符串到服务器
 */
int tcp_client_send_json(const char *json_text)
{
    uint32_t json_len;
    uint32_t net_len;
    int fd;
    int ret;

    if (json_text == NULL) {
        return -1;
    }

    json_len = (uint32_t)strlen(json_text);
    if (json_len == 0 || json_len > TCP_CLIENT_MAX_JSON_LEN) {
        LOG_WARN("tcp client send invalid json length: %u", json_len);
        return -1;
    }

    pthread_mutex_lock(&g_socket_mutex);

    fd = g_socket_fd;
    if (fd < 0) {
        pthread_mutex_unlock(&g_socket_mutex);
        return -1;
    }

    net_len = htonl(json_len);
    ret = send_all_raw(fd, &net_len, sizeof(net_len));
    if (ret == 0) {
        ret = send_all_raw(fd, json_text, (int)json_len);
    }

    pthread_mutex_unlock(&g_socket_mutex);

    return ret;
}

/*
 * @brief 检查 TCP 客户端是否已连接
 */
int tcp_client_is_connected(void)
{
    int connected;

    pthread_mutex_lock(&g_socket_mutex);
    connected = (g_socket_fd >= 0);
    pthread_mutex_unlock(&g_socket_mutex);

    return connected;
}
