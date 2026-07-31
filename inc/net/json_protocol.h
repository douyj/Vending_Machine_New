#ifndef __JSON_PROTOCOL_H__
#define __JSON_PROTOCOL_H__

#include "order/order_manager.h"

#define JSON_PROTOCOL_VERSION 1
#define JSON_PROTOCOL_DEVICE_ID "VM001"

typedef enum {
    JSON_PROTO_OK = 0,
    JSON_PROTO_ERR_INVALID_JSON = -1,
    JSON_PROTO_ERR_INVALID_FIELD = -2,
    JSON_PROTO_ERR_UNKNOWN_CMD = -3,
    JSON_PROTO_ERR_INTERNAL = -4
} json_proto_error_t;

/*
 * @brief 处理 Qt 后台发来的完整 JSON 请求
 *
 * @param request_json 请求 JSON 字符串
 * @return 响应 JSON 字符串。返回值由 cJSON 分配，使用完成后必须调用
 *         json_protocol_free() 释放。
 */
char *json_protocol_handle_request(const char *request_json);

/*
 * @brief 构造售货机主动上报给 Qt 后台的事件 JSON
 *
 * @return 事件 JSON 字符串。返回值使用完成后必须调用 json_protocol_free() 释放。
 */
char *json_protocol_build_heartbeat(void);
char *json_protocol_build_order_created(const order_info_t *order);
char *json_protocol_build_stock_changed(int product_id, int stock);

/*
 * @brief 释放 json_protocol 模块返回的 JSON 字符串
 */
void json_protocol_free(char *json_text);

#endif
