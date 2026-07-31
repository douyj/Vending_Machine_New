#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

/*
 * @brief 启动 TCP 客户端
 *
 * 售货机作为客户端，主动连接 Qt 后台 TCP Server。
 * 函数内部会创建后台线程，不会阻塞 LVGL 主循环。
 *
 * @param server_ip Qt 后台服务器 IP，例如 "127.0.0.1"
 * @param server_port Qt 后台监听端口，例如 9000
 * @return 0 成功，-1 失败
 */
int tcp_client_start(const char *server_ip, int server_port);

/*
 * @brief 停止 TCP 客户端
 */
void tcp_client_stop(void);

/*
 * @brief 发送一条 JSON 消息
 *
 * 内部会自动发送 4 字节网络字节序长度头，再发送 JSON 正文。
 *
 * @param json_text JSON 字符串
 * @return 0 成功，-1 失败
 */
int tcp_client_send_json(const char *json_text);

/*
 * @brief 查询当前是否已连接 Qt 后台
 *
 * @return 1 已连接，0 未连接
 */
int tcp_client_is_connected(void);

#endif
