#include "net/json_protocol.h"

#include "device/door_manager.h"
#include "device/door_state.h"
#include "member/member_manager.h"
#include "product/product_manager.h"
#include "third_party/cJSON.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

static int g_event_seq = 1;

/*
 * @brief 创建协议公共字段
 * @param type 消息类型，如 "response" 或 "event"
 * @param seq 消息序号，用于匹配请求和响应
 * @param cmd 命令，用于指定具体操作，如 "create_order" 或 "get_stock"
 *
 * 所有响应和事件都带 version/type/seq/cmd/device_id，方便 Qt 后台统一解析。
 */
static cJSON *create_base_message(const char *type, int seq, const char *cmd)
{
    cJSON *root = cJSON_CreateObject(); //创建json空对象

    if (root == NULL) {
        return NULL;
    }

    cJSON_AddNumberToObject(root, "version", JSON_PROTOCOL_VERSION);
    cJSON_AddStringToObject(root, "type", type);
    cJSON_AddNumberToObject(root, "seq", seq);
    cJSON_AddStringToObject(root, "cmd", cmd);
    cJSON_AddStringToObject(root, "device_id", JSON_PROTOCOL_DEVICE_ID);

    return root;
}

/*
 * @brief 把 cJSON 对象转换成真正可以发送的 JSON 字符串，然后把原来的 cJSON 对象释放掉，最后返回字符串
 * @param root cJSON 对象树的根节点
 * @return 输出的 JSON 字符串。返回值由 cJSON 分配，使用完成后必须调用
 *         json_protocol_free() 释放。
 */
static char *print_and_delete(cJSON *root)
{
    char *json_text;

    if (root == NULL) {
        return NULL;
    }

    json_text = cJSON_PrintUnformatted(root);   //把json对象转换成字符串，不包含换行符
    cJSON_Delete(root);

    return json_text;
}


/*
 * @brief 构造统一响应消息
 * @param seq 嶈息序号，用于匹配请求和响应
 * @param cmd 命令，用于指定具体操作，如 "create_order" 或 "get_stock"
 * @param code 错误码，0 成功，其他失败
 * @param message 错误信息，用于描述失败原因
 * @param data 响应数据，用于包含具体数据
 *
 * 响应里保留原 cmd 和 seq，Qt 后台可以用它们匹配请求。
 */
static char *build_response(int seq,
                            const char *cmd,
                            int code,
                            const char *message,
                            cJSON *data)
{
    cJSON *root = create_base_message("response", seq, cmd != NULL ? cmd : "");

    if (root == NULL) {
        cJSON_Delete(data);
        return NULL;
    }

    cJSON_AddNumberToObject(root, "code", code);
    cJSON_AddStringToObject(root, "message", message != NULL ? message : "");

    if (data != NULL) {
        cJSON_AddItemToObject(root, "data", data);
    } else {
        cJSON_AddItemToObject(root, "data", cJSON_CreateObject());
    }

    return print_and_delete(root);
}

/*
 * @brief 从 JSON 对象中读取整数字段
 * @param root JSON 对象树的根节点
 * @param name 字段名
 * @param out_value 输出整数值
 * @return 0 成功，-1 失败
 *         失败时，out_value 未被修改
 */
static int get_json_int(cJSON *root, const char *name, int *out_value)
{
    cJSON *item;

    if (root == NULL || name == NULL || out_value == NULL) {
        return -1;
    }

    item = cJSON_GetObjectItemCaseSensitive(root, name);
    if (!cJSON_IsNumber(item)) {
        return -1;
    }

    *out_value = item->valueint;
    return 0;
}

/*
 * @brief 从 JSON 对象中读取字符串字段
 * @param root JSON 对象树的根节点
 * @param name 字段名
 * @return 字符值，或 NULL 表示失败
 */
static const char *get_json_string(cJSON *root, const char *name)
{
    cJSON *item;

    if (root == NULL || name == NULL) {
        return NULL;
    }

    item = cJSON_GetObjectItemCaseSensitive(root, name);
    if (!cJSON_IsString(item) || item->valuestring == NULL) {
        return NULL;
    }

    return item->valuestring;
}

/*
 * @brief 处理 device.status，读取当前设备状态，比如门状态、会员是否登录、当前会员信息，然后组装成 JSON 响应返回
 * @param seq 消息序号，用于匹配请求和响应
 * @return 响应 JSON 字符串，包含门状态、会员是否登录、当前会员信息
 */
static char *handle_device_status(int seq)
{
    cJSON *data = cJSON_CreateObject();
    member_info_t member;

    if (data == NULL) {
        return build_response(seq, "device.status", JSON_PROTO_ERR_INTERNAL, "internal error", NULL);
    }

    cJSON_AddStringToObject(data, "door_state", door_state_to_string(door_get_state()));
    cJSON_AddNumberToObject(data, "member_logged_in", member_is_logged_in());

    if (member_get_current(&member) == MEMBER_ERR_OK) {
        cJSON_AddNumberToObject(data, "member_id", member.member_id);
        cJSON_AddStringToObject(data, "member_name", member.member_name);
        cJSON_AddNumberToObject(data, "balance", member.balance);
    }

    return build_response(seq, "device.status", JSON_PROTO_OK, "ok", data);
}

/*
 * @brief 处理 product.list，从商品管理模块里拿到所有商品，然后把每个商品转成 JSON 对象，装进 JSON 数组，最后返回给 Qt 后台
 * @param seq 消息序号，用于匹配请求和响应
 * @return 响应 JSON 字符串，包含所有商品信息
 */
static char *handle_product_list(int seq)
{
    product_info_t products[PRODUCT_MANAGER_PRODUCT_COUNT];
    cJSON *data = cJSON_CreateObject();
    cJSON *array = cJSON_CreateArray();
    int count;
    int i;

    if (data == NULL || array == NULL) {
        cJSON_Delete(data);
        cJSON_Delete(array);
        return build_response(seq, "product.list", JSON_PROTO_ERR_INTERNAL, "internal error", NULL);
    }

    count = product_manager_get_all(products, PRODUCT_MANAGER_PRODUCT_COUNT);
    if (count < 0) {
        cJSON_Delete(data);
        cJSON_Delete(array);
        return build_response(seq, "product.list", JSON_PROTO_ERR_INTERNAL, "get product list failed", NULL);
    }

    for (i = 0; i < count; i++) {
        cJSON *item = cJSON_CreateObject();
        if (item == NULL) {
            cJSON_Delete(data);
            cJSON_Delete(array);
            return build_response(seq, "product.list", JSON_PROTO_ERR_INTERNAL, "internal error", NULL);
        }

        cJSON_AddNumberToObject(item, "id", products[i].product_id);
        cJSON_AddStringToObject(item, "name", products[i].product_name);
        cJSON_AddNumberToObject(item, "price", products[i].product_price);
        cJSON_AddNumberToObject(item, "stock", products[i].product_stock);
        cJSON_AddNumberToObject(item, "category", products[i].product_category);
        cJSON_AddItemToArray(array, item);
    }

    cJSON_AddItemToObject(data, "products", array);
    return build_response(seq, "product.list", JSON_PROTO_OK, "ok", data);
}

/*
 * @brief 处理 product.update_stock，后台直接设置指定商品库存
 * @param seq 消息序号，用于匹配请求和响应
 * @param data 请求 JSON 对象，包含 product_id 和 stock 字段
 * @return 响应 JSON 字符串，包含 ok 字段
 */
static char *handle_product_update_stock(int seq, cJSON *data)
{
    int product_id;
    int stock;
    int ret;

    if (get_json_int(data, "product_id", &product_id) != 0 ||
        get_json_int(data, "stock", &stock) != 0) {
        return build_response(seq,
                              "product.update_stock",
                              JSON_PROTO_ERR_INVALID_FIELD,
                              "missing product_id or stock",
                              NULL);
    }

    ret = product_manager_set_stock(product_id, stock);
    if (ret != 0) {
        return build_response(seq, "product.update_stock", JSON_PROTO_ERR_INTERNAL, "set stock failed", NULL);
    }

    return build_response(seq, "product.update_stock", JSON_PROTO_OK, "ok", NULL);
}

/*
 * @brief 处理 door.open，后台远程开门
 */
static char *handle_door_open(int seq)
{
    int ret = door_open("remote command");

    if (ret != DOOR_ERR_OK) {
        return build_response(seq, "door.open", ret, door_error_to_string(ret), NULL);
    }

    return build_response(seq, "door.open", JSON_PROTO_OK, "ok", NULL);
}

/*
 * @brief 处理 door.close，后台远程关门
 */
static char *handle_door_close(int seq)
{
    int ret = door_close("remote command");

    if (ret != DOOR_ERR_OK) {
        return build_response(seq, "door.close", ret, door_error_to_string(ret), NULL);
    }

    return build_response(seq, "door.close", JSON_PROTO_OK, "ok", NULL);
}

/*
 * @brief 解析后台请求，根据 cmd 分发到对应业务处理函数
 * @param request_json 请求 JSON 字符串
 * @return 响应 JSON 字符串
 */
char *json_protocol_handle_request(const char *request_json)
{
    cJSON *root;
    cJSON *data;
    const char *type;
    const char *cmd;
    int version;
    int seq = 0;
    char *response;

    if (request_json == NULL) {
        return build_response(0, "", JSON_PROTO_ERR_INVALID_FIELD, "request is null", NULL);
    }

    // 把 JSON 字符串解析成 cJSON 对象
    root = cJSON_Parse(request_json);
    if (root == NULL) {
        return build_response(0, "", JSON_PROTO_ERR_INVALID_JSON, "invalid json", NULL);
    }

    if (get_json_int(root, "seq", &seq) != 0) {
        seq = 0;
    }

    if (get_json_int(root, "version", &version) != 0 ||
        version != JSON_PROTOCOL_VERSION) {
        cJSON_Delete(root);
        return build_response(seq,
                              "",
                              JSON_PROTO_ERR_INVALID_FIELD,
                              "invalid version",
                              NULL);
    }

    type = get_json_string(root, "type");
    cmd = get_json_string(root, "cmd");
    data = cJSON_GetObjectItemCaseSensitive(root, "data");

    if (type == NULL || strcmp(type, "request") != 0 || cmd == NULL) {
        cJSON_Delete(root);
        return build_response(seq,
                              "",
                              JSON_PROTO_ERR_INVALID_FIELD,
                              "invalid type or cmd",
                              NULL);
    }

    if (data != NULL && !cJSON_IsObject(data)) {
        cJSON_Delete(root);
        return build_response(seq,
                              cmd != NULL ? cmd : "",
                              JSON_PROTO_ERR_INVALID_FIELD,
                              "data must be object",
                              NULL);
    }

    if (strcmp(cmd, "device.status") == 0) {
        response = handle_device_status(seq);
    } else if (strcmp(cmd, "product.list") == 0) {
        response = handle_product_list(seq);
    } else if (strcmp(cmd, "product.update_stock") == 0) {
        response = handle_product_update_stock(seq, data);
    } else if (strcmp(cmd, "door.open") == 0) {
        response = handle_door_open(seq);
    } else if (strcmp(cmd, "door.close") == 0) {
        response = handle_door_close(seq);
    } else {
        response = build_response(seq,
                                  cmd,
                                  JSON_PROTO_ERR_UNKNOWN_CMD,
                                  "unknown cmd",
                                  NULL);
    }

    cJSON_Delete(root);
    return response;
}

/*
 * @brief 构造设备心跳事件
 */
char *json_protocol_build_heartbeat(void)
{
    cJSON *root = create_base_message("event", g_event_seq++, "device.heartbeat");
    cJSON *data = cJSON_CreateObject();

    if (root == NULL || data == NULL) {
        cJSON_Delete(root);
        cJSON_Delete(data);
        return NULL;
    }

    cJSON_AddStringToObject(data, "status", "online");
    cJSON_AddStringToObject(data, "door_state", door_state_to_string(door_get_state()));
    cJSON_AddNumberToObject(data, "timestamp", (double)time(NULL));
    cJSON_AddItemToObject(root, "data", data);

    return print_and_delete(root);
}

/*
 * @brief 构造订单创建/完成后的上报事件
 */
char *json_protocol_build_order_created(const order_info_t *order)
{
    cJSON *root;
    cJSON *data;

    if (order == NULL) {
        return NULL;
    }

    root = create_base_message("event", g_event_seq++, "order.created");
    data = cJSON_CreateObject();

    if (root == NULL || data == NULL) {
        cJSON_Delete(root);
        cJSON_Delete(data);
        return NULL;
    }

    cJSON_AddStringToObject(data, "order_id", order->order_id);
    cJSON_AddNumberToObject(data, "member_id", order->member_id);
    cJSON_AddStringToObject(data, "member_name", order->member_name);
    cJSON_AddNumberToObject(data, "product_id", order->product_id);
    cJSON_AddStringToObject(data, "product_name", order->product_name);
    cJSON_AddNumberToObject(data, "product_count", order->product_count);
    cJSON_AddNumberToObject(data, "unit_price", order->unit_price);
    cJSON_AddNumberToObject(data, "total_price", order->total_price);
    cJSON_AddNumberToObject(data, "balance_before_pay", order->balance_before_pay);
    cJSON_AddNumberToObject(data, "balance_after_pay", order->balance_after_pay);
    cJSON_AddStringToObject(data, "state", order_state_to_string(order->state));
    cJSON_AddNumberToObject(data, "create_time", (double)order->create_time);
    cJSON_AddItemToObject(root, "data", data);

    return print_and_delete(root);
}

/*
 * @brief 构造库存变化上报事件
 */
char *json_protocol_build_stock_changed(int product_id, int stock)
{
    cJSON *root = create_base_message("event", g_event_seq++, "product.stock_changed");
    cJSON *data = cJSON_CreateObject();

    if (root == NULL || data == NULL) {
        cJSON_Delete(root);
        cJSON_Delete(data);
        return NULL;
    }

    cJSON_AddNumberToObject(data, "product_id", product_id);
    cJSON_AddNumberToObject(data, "stock", stock);
    cJSON_AddNumberToObject(data, "timestamp", (double)time(NULL));
    cJSON_AddItemToObject(root, "data", data);

    return print_and_delete(root);
}

/*
 * @brief 释放 cJSON_PrintUnformatted() 返回的字符串
 */
void json_protocol_free(char *json_text)
{
    free(json_text);
}
