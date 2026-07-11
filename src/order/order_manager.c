#include "order/order_manager.h"
#include "order/order_state.h"

#include "product/product_manager.h"
#include "storage/storage_manager.h"
#include "member/member_manager.h"

#include "log/log.h"

#include <stdio.h>
#include <string.h>
#include <time.h>


/*
 * 订单序列号
 * 第一版先用内存自增。
 * 后面接 SQLite 后，可以从数据库读取最大订单号继续递增。
 */
static int g_order_seq = 1;


/*
 * @brief 生成订单号
 * @param buf 输出缓冲区
 * @param len 缓冲区长度
 */
static void order_generate_id(char *buf, int len)
{
    if (buf == NULL || len <= 0) {
        return;
    }

    snprintf(buf, len, "ORDER_%06d", g_order_seq++);
}

static void order_append_cart_item_text(char *buf,
                                        int buf_len,
                                        const char *product_name,
                                        int quantity)
{
    int used;

    if (buf == NULL || buf_len <= 0 || product_name == NULL || quantity <= 0) {
        return;
    }

    used = (int)strlen(buf);
    if (used >= buf_len - 1) {
        return;
    }

    snprintf(buf + used,
             buf_len - used,
             "%s%s x%d",
             used > 0 ? "\n" : "",
             product_name,
             quantity);
}


/*
 * @brief 初始化订单管理器
 * @return ORDER_ERR_OK 成功，其他失败
 */
int order_manager_init(void)
{
    g_order_seq = storage_get_next_order_seq();

    LOG_INFO("order manager initialized, next_order_seq=%d", g_order_seq);

    return ORDER_ERR_OK;
}


/*
 * @brief 创建订单
 *
 * 注意：
 * 这里只负责“生成订单信息”，不扣余额，不扣库存。
 *
 * 创建订单时会做：
 * 1. 参数检查
 * 2. 检查会员是否登录
 * 3. 获取当前会员信息
 * 4. 查询商品信息
 * 5. 检查库存是否足够
 * 6. 填充订单结构体
 *
 * @param product_id 商品 ID
 * @param count 商品数量
 * @param out_order 输出订单信息
 * @return ORDER_ERR_OK 成功，其他失败
 */
int order_create(int product_id, int count, order_info_t *out_order)
{
    int ret;
    product_info_t product;
    member_info_t member;

    if (product_id <= 0 || count <= 0 || out_order == NULL) {
        LOG_WARN("order create invalid param, product_id: %d, count: %d",
                 product_id,
                 count);
        return ORDER_ERR_INVALID_PARAM;
    }

    /*
     * 1. 检查会员是否登录
     * 商品选购必须发生在会员登录之后。
     */
    if (!member_is_logged_in()) {
        LOG_WARN("order create failed, member not logged in");
        return ORDER_ERR_MEMBER_NOT_LOGIN;
    }

    /*
     * 2. 获取当前会员信息
     */
    ret = member_get_current(&member);
    if (ret != MEMBER_ERR_OK) {
        LOG_WARN("order create failed, get current member failed, ret=%d", ret);
        return ORDER_ERR_MEMBER_NOT_LOGIN;
    }

    /*
     * 3. 查询商品信息
     */
    ret = product_manager_get_by_id(product_id, &product);
    if (ret != 0) {
        LOG_WARN("order create failed, product not found, product_id: %d",
                 product_id);
        return ORDER_ERR_PRODUCT_NOT_FOUND;
    }

    /*
     * 4. 检查库存
     */
    if (product.product_stock < count) {
        LOG_WARN("order create failed, stock not enough, product_id: %d, stock: %d, need: %d",
                 product_id,
                 product.product_stock,
                 count);
        return ORDER_ERR_OUT_OF_STOCK;
    }

    /*
     * 5. 填充订单
     */
    memset(out_order, 0, sizeof(order_info_t));

    order_generate_id(out_order->order_id, ORDER_ID_MAX_LEN);

    out_order->member_id = member.member_id;
    snprintf(out_order->member_name,
             sizeof(out_order->member_name),
             "%s",
             member.member_name);

    out_order->product_id = product.product_id;
    snprintf(out_order->product_name,
             sizeof(out_order->product_name),
             "%s",
             product.product_name);

    out_order->product_count = count;
    out_order->unit_price = product.product_price;
    out_order->total_price = product.product_price * count;

    /*
     * 创建订单时还没有真正支付。
     * 所以 before / after 先都填当前余额。
     * 真正扣款在 order_pay_by_member_balance() 里更新。
     */
    out_order->balance_before_pay = member.balance;
    out_order->balance_after_pay = member.balance;

    out_order->state = ORDER_STATE_CREATED;

    /*
     * upload_status:
     * 0 = 未上传 / 待上传
     * 1 = 已上传
     *
     * 第一版还没有 MQTT / SQLite，所以先默认 0。
     */
    out_order->upload_status = 0;
    out_order->create_time = time(NULL);

    LOG_INFO("order create success, order_id: %s, member_id: %d, member_name: %s, product_id: %d, product_name: %s, count: %d, total_price: %.2f, state: %s",
             out_order->order_id,
             out_order->member_id,
             out_order->member_name,
             out_order->product_id,
             out_order->product_name,
             out_order->product_count,
             out_order->total_price,
             order_state_to_string(out_order->state));

    return ORDER_ERR_OK;
}


/*
 * @brief 检查会员余额是否足够
 *
 * 注意：
 * 这里只检查，不扣钱。
 *
 * @param order 订单信息
 * @return ORDER_ERR_OK 成功，其他失败
 */
int order_check_balance(order_info_t *order)
{
    int ret;

    if (order == NULL) {
        LOG_WARN("order check balance invalid param, order is NULL");
        return ORDER_ERR_INVALID_PARAM;
    }

    if (order->total_price <= 0) {
        LOG_WARN("order check balance failed, invalid total_price: %.2f",
                 order->total_price);
        order->state = ORDER_STATE_FAILED;
        return ORDER_ERR_INVALID_PARAM;
    }

    /*
     * 订单进入检查余额状态
     */
    order->state = ORDER_STATE_CHECKING_BALANCE;

    ret = member_check_balance(order->total_price);
    if (ret != MEMBER_ERR_OK) {
        LOG_WARN("order check balance failed, order_id: %s, need: %.2f, ret=%d",
                 order->order_id,
                 order->total_price,
                 ret);

        order->state = ORDER_STATE_FAILED;

        if (ret == MEMBER_ERR_NOT_LOGIN) {
            return ORDER_ERR_MEMBER_NOT_LOGIN;
        }

        if (ret == MEMBER_ERR_BALANCE_NOT_ENOUGH) {
            return ORDER_ERR_BALANCE_NOT_ENOUGH;
        }

        return ORDER_ERR_PAY_FAILED;
    }

    LOG_INFO("order check balance success, order_id: %s, need: %.2f, state: %s",
             order->order_id,
             order->total_price,
             order_state_to_string(order->state));

    return ORDER_ERR_OK;
}


/*
 * @brief 使用会员余额支付订单
 *
 * 这里才真正扣会员余额。
 *
 * 流程：
 * 1. 检查参数
 * 2. 状态切到 PAYING
 * 3. 调用 member_deduct_balance()
 * 4. 保存支付前余额和支付后余额
 * 5. 状态切到 PAID
 *
 * @param order 订单信息
 * @return ORDER_ERR_OK 成功，其他失败
 */
int order_pay_by_member_balance(order_info_t *order)
{
    int ret;
    double before = 0.0;
    double after = 0.0;

    if (order == NULL) {
        LOG_WARN("order pay invalid param, order is NULL");
        return ORDER_ERR_INVALID_PARAM;
    }

    if (order->total_price <= 0) {
        LOG_WARN("order pay failed, invalid total_price: %.2f",
                 order->total_price);
        order->state = ORDER_STATE_FAILED;
        return ORDER_ERR_INVALID_PARAM;
    }

    /*
     * 只有 CREATED 或 CHECKING_BALANCE 之后的订单才能支付。
     * 第一版不强制太死，但至少不能是 DONE / FAILED。
     */
    if (order->state == ORDER_STATE_DONE ||
        order->state == ORDER_STATE_FAILED) {
        LOG_WARN("order pay failed, invalid state, order_id: %s, state: %s",
                 order->order_id,
                 order_state_to_string(order->state));
        return ORDER_ERR_INVALID_PARAM;
    }

    order->state = ORDER_STATE_PAYING;

    ret = member_deduct_balance(order->total_price, &before, &after);
    if (ret != MEMBER_ERR_OK) {
        LOG_WARN("order pay failed, member deduct balance failed, order_id: %s, ret=%d",
                 order->order_id,
                 ret);

        order->state = ORDER_STATE_FAILED;

        if (ret == MEMBER_ERR_NOT_LOGIN) {
            return ORDER_ERR_MEMBER_NOT_LOGIN;
        }

        if (ret == MEMBER_ERR_BALANCE_NOT_ENOUGH) {
            return ORDER_ERR_BALANCE_NOT_ENOUGH;
        }

        return ORDER_ERR_PAY_FAILED;
    }

    order->balance_before_pay = before;
    order->balance_after_pay = after;

    order->state = ORDER_STATE_PAID;

    LOG_INFO("order pay success, order_id: %s, pay_amount: %.2f, before: %.2f, after: %.2f, state: %s",
             order->order_id,
             order->total_price,
             order->balance_before_pay,
             order->balance_after_pay,
             order_state_to_string(order->state));

    return ORDER_ERR_OK;
}


/*
 * @brief 扣减商品库存
 *
 * 注意：
 * 只有订单已经 PAID 后，才允许扣库存。
 *
 * @param order 订单信息
 * @return ORDER_ERR_OK 成功，其他失败
 */
int order_deduct_stock(order_info_t *order)
{
    int ret;

    if (order == NULL) {
        LOG_WARN("order deduct stock invalid param, order is NULL");
        return ORDER_ERR_INVALID_PARAM;
    }

    if (order->product_id <= 0 || order->product_count <= 0) {
        LOG_WARN("order deduct stock invalid param, order_id: %s, product_id: %d, product_count: %d",
                 order->order_id,
                 order->product_id,
                 order->product_count);

        order->state = ORDER_STATE_FAILED;
        return ORDER_ERR_INVALID_PARAM;
    }

    if (order->state != ORDER_STATE_PAID) {
        LOG_WARN("order deduct stock failed, order not paid, order_id: %s, state: %s",
                 order->order_id,
                 order_state_to_string(order->state));

        order->state = ORDER_STATE_FAILED;
        return ORDER_ERR_PAY_FAILED;
    }

    ret = product_manager_sub_stock(order->product_id, order->product_count);
    if (ret != 0) {
        LOG_WARN("order deduct stock failed, order_id: %s, product_id: %d, count: %d, ret=%d",
                 order->order_id,
                 order->product_id,
                 order->product_count,
                 ret);

        order->state = ORDER_STATE_FAILED;
        return ORDER_ERR_STOCK_DEDUCT_FAILED;
    }

    order->state = ORDER_STATE_STOCK_DEDUCTED;

    LOG_INFO("order deduct stock success, order_id: %s, product_id: %d, count: %d, state: %s",
             order->order_id,
             order->product_id,
             order->product_count,
             order_state_to_string(order->state));

    return ORDER_ERR_OK;
}


/*
 * @brief 完成订单
 *
 * 只有库存已经扣减成功，订单才能 DONE。
 *
 * 后面可以在这里扩展：
 * 1. 写 SQLite
 * 2. 写余额流水
 * 3. 写库存变化
 * 4. MQTT 上报订单
 * 5. 网络断开则设置 PENDING_UPLOAD
 *
 * @param order 订单信息
 * @return ORDER_ERR_OK 成功，其他失败
 */
int order_finish(order_info_t *order)
{
    int ret;

    if (order == NULL) {
        LOG_WARN("order finish invalid param, order is NULL");
        return ORDER_ERR_INVALID_PARAM;
    }

    if (order->state != ORDER_STATE_STOCK_DEDUCTED) {
        LOG_WARN("order finish failed, invalid state, order_id: %s, state: %s",
                 order->order_id,
                 order_state_to_string(order->state));

        order->state = ORDER_STATE_FAILED;
        return ORDER_ERR_INVALID_PARAM;
    }

    order->state = ORDER_STATE_DONE;

    /*
     * 第一版还没有 MQTT / SQLite。
     * upload_status = 0 表示订单已完成，但还未上传后台。
     */
    order->upload_status = 0;

    ret = storage_insert_order(order);
    if (ret != STORAGE_ERR_OK) {
        LOG_WARN("order finish failed, insert order failed, order_id: %s, ret=%d",
                 order->order_id,
                 ret);
        order->state = ORDER_STATE_FAILED;
        return ORDER_ERR_PAY_FAILED;
    }

    LOG_INFO("order finish success, order_id: %s, member_id: %d, product_id: %d, count: %d, total_price: %.2f, balance_before: %.2f, balance_after: %.2f, state: %s",
             order->order_id,
             order->member_id,
             order->product_id,
             order->product_count,
             order->total_price,
             order->balance_before_pay,
             order->balance_after_pay,
             order_state_to_string(order->state));

    return ORDER_ERR_OK;
}


/*
 * @brief 完整购买流程
 *
 * 这是 order_manager 的核心函数。
 *
 * 完整流程：
 * 1. 创建订单
 * 2. 检查余额
 * 3. 会员余额支付
 * 4. 扣商品库存
 * 5. 订单完成
 *
 * 注意：
 * 第一版这里不直接调用 door_close()。
 * 因为 door_manager 还没接进来。
 *
 * 后面接入 door_manager 后，可以在成功后调用：
 *
 *     door_close();
 *
 * 如果失败也希望自动关门，可以在失败分支调用：
 *
 *     door_close();
 *
 * @param product_id 商品 ID
 * @param count 商品数量
 * @param out_order 输出订单
 * @return ORDER_ERR_OK 成功，其他失败
 */
int order_process_buy(int product_id, int count, order_info_t *out_order)
{
    int ret;

    if (out_order == NULL) {
        LOG_WARN("order process buy invalid param, out_order is NULL");
        return ORDER_ERR_INVALID_PARAM;
    }

    /*
     * 1. 创建订单
     */
    ret = order_create(product_id, count, out_order);
    if (ret != ORDER_ERR_OK) {
        LOG_WARN("order process buy failed at create order, product_id: %d, count: %d, ret=%d",
                 product_id,
                 count,
                 ret);
        return ret;
    }

    /*
     * 2. 检查余额
     */
    ret = order_check_balance(out_order);
    if (ret != ORDER_ERR_OK) {
        LOG_WARN("order process buy failed at check balance, order_id: %s, ret=%d",
                 out_order->order_id,
                 ret);

        out_order->state = ORDER_STATE_FAILED;
        return ret;
    }

    /*
     * 3. 会员余额支付
     */
    ret = order_pay_by_member_balance(out_order);
    if (ret != ORDER_ERR_OK) {
        LOG_WARN("order process buy failed at pay, order_id: %s, ret=%d",
                 out_order->order_id,
                 ret);

        out_order->state = ORDER_STATE_FAILED;
        return ret;
    }

    /*
     * 4. 扣商品库存
     */
    ret = order_deduct_stock(out_order);
    if (ret != ORDER_ERR_OK) {
        LOG_WARN("order process buy failed at deduct stock, order_id: %s, ret=%d",
                 out_order->order_id,
                 ret);

        out_order->state = ORDER_STATE_FAILED;
        return ret;
    }

    /*
     * 5. 订单完成
     */
    ret = order_finish(out_order);
    if (ret != ORDER_ERR_OK) {
        LOG_WARN("order process buy failed at finish, order_id: %s, ret=%d",
                 out_order->order_id,
                 ret);

        out_order->state = ORDER_STATE_FAILED;
        return ret;
    }

    LOG_INFO("order process buy success, order_id: %s, member_id: %d, product_name: %s, count: %d, total_price: %.2f, state: %s",
             out_order->order_id,
             out_order->member_id,
             out_order->product_name,
             out_order->product_count,
             out_order->total_price,
             order_state_to_string(out_order->state));

    return ORDER_ERR_OK;
}


/*
    @brief 购物车购买流程
    @param items 购物车商品数组
    @param item_count 购物车商品数量
    @param out_paid_total 输出已支付金额
    @param out_balance_after 输出余额
    @return ORDER_ERR_OK 成功，其他失败
*/
int order_process_cart_buy(const order_cart_item_t *items,
                           int item_count,
                           double *out_paid_total,
                           double *out_balance_after)
{
    product_info_t products[PRODUCT_MANAGER_PRODUCT_COUNT];
    order_info_t order;
    member_info_t member;
    double total_price = 0.0;
    double balance_before = 0.0;
    double balance_after = 0.0;
    int total_quantity = 0;
    int ret;

    if (items == NULL || item_count <= 0 ||
        item_count > PRODUCT_MANAGER_PRODUCT_COUNT) {
        LOG_WARN("cart buy invalid param, item_count=%d", item_count);
        return ORDER_ERR_INVALID_PARAM;
    }

    if (!member_is_logged_in()) {
        LOG_WARN("cart buy failed, member not logged in");
        return ORDER_ERR_MEMBER_NOT_LOGIN;
    }

    memset(products, 0, sizeof(products));
    memset(&order, 0, sizeof(order));

    for (int i = 0; i < item_count; i++) {
        if (items[i].product_id <= 0 || items[i].quantity <= 0) {
            LOG_WARN("cart buy invalid item, index=%d, product_id=%d, quantity=%d",
                     i,
                     items[i].product_id,
                     items[i].quantity);
            return ORDER_ERR_INVALID_PARAM;
        }

        ret = product_manager_get_by_id(items[i].product_id, &products[i]);
        if (ret != 0) {
            LOG_WARN("cart buy failed, product not found, product_id=%d",
                     items[i].product_id);
            return ORDER_ERR_PRODUCT_NOT_FOUND;
        }

        if (products[i].product_stock < items[i].quantity) {
            LOG_WARN("cart buy failed, stock not enough, product_id=%d, stock=%d, need=%d",
                     items[i].product_id,
                     products[i].product_stock,
                     items[i].quantity);
            return ORDER_ERR_OUT_OF_STOCK;
        }

        total_price += products[i].product_price * items[i].quantity;
        total_quantity += items[i].quantity;
    }

    if (total_price <= 0.0) {
        LOG_WARN("cart buy failed, invalid total_price=%.2f", total_price);
        return ORDER_ERR_INVALID_PARAM;
    }

    ret = member_check_balance(total_price);
    if (ret != MEMBER_ERR_OK) {
        LOG_WARN("cart buy failed, balance check failed, total=%.2f, ret=%d",
                 total_price,
                 ret);
        if (ret == MEMBER_ERR_NOT_LOGIN) {
            return ORDER_ERR_MEMBER_NOT_LOGIN;
        }
        if (ret == MEMBER_ERR_BALANCE_NOT_ENOUGH) {
            return ORDER_ERR_BALANCE_NOT_ENOUGH;
        }
        return ORDER_ERR_PAY_FAILED;
    }

    ret = member_get_current(&member);
    if (ret != MEMBER_ERR_OK) {
        LOG_WARN("cart buy failed, get current member failed, ret=%d", ret);
        return ORDER_ERR_MEMBER_NOT_LOGIN;
    }

    order_generate_id(order.order_id, ORDER_ID_MAX_LEN);
    order.member_id = member.member_id;
    snprintf(order.member_name, sizeof(order.member_name), "%s", member.member_name);
    order.product_id = 0;
    order.product_count = total_quantity;
    order.unit_price = 0.0;
    order.total_price = total_price;
    order.balance_before_pay = member.balance;
    order.balance_after_pay = member.balance;
    order.state = ORDER_STATE_CREATED;
    order.upload_status = 0;
    order.create_time = time(NULL);

    for (int i = 0; i < item_count; i++) {
        order_append_cart_item_text(order.product_name,
                                    sizeof(order.product_name),
                                    products[i].product_name,
                                    items[i].quantity);
    }

    ret = member_deduct_balance(total_price, &balance_before, &balance_after);
    if (ret != MEMBER_ERR_OK) {
        LOG_WARN("cart buy failed, deduct balance failed, total=%.2f, ret=%d",
                 total_price,
                 ret);
        if (ret == MEMBER_ERR_NOT_LOGIN) {
            return ORDER_ERR_MEMBER_NOT_LOGIN;
        }
        if (ret == MEMBER_ERR_BALANCE_NOT_ENOUGH) {
            return ORDER_ERR_BALANCE_NOT_ENOUGH;
        }
        return ORDER_ERR_PAY_FAILED;
    }

    order.balance_before_pay = balance_before;
    order.balance_after_pay = balance_after;
    order.state = ORDER_STATE_PAID;

    for (int i = 0; i < item_count; i++) {
        ret = product_manager_sub_stock(items[i].product_id, items[i].quantity);
        if (ret != 0) {
            LOG_WARN("cart buy failed, deduct stock failed, order_id=%s, product_id=%d, quantity=%d, ret=%d",
                     order.order_id,
                     items[i].product_id,
                     items[i].quantity,
                     ret);
            order.state = ORDER_STATE_FAILED;
            return ORDER_ERR_STOCK_DEDUCT_FAILED;
        }
    }

    order.state = ORDER_STATE_STOCK_DEDUCTED;

    ret = order_finish(&order);
    if (ret != ORDER_ERR_OK) {
        LOG_WARN("cart buy failed, finish order failed, order_id=%s, ret=%d",
                 order.order_id,
                 ret);
        return ret;
    }

    if (out_paid_total != NULL) {
        *out_paid_total = total_price;
    }

    if (out_balance_after != NULL) {
        *out_balance_after = balance_after;
    }

    LOG_INFO("cart buy success, order_id=%s, item_count=%d, total_price=%.2f, balance_before=%.2f, balance_after=%.2f",
             order.order_id,
             item_count,
             total_price,
             balance_before,
             balance_after);

    return ORDER_ERR_OK;

}


/*
 * @brief 订单错误码转字符串
 *
 * 方便日志、调试、Qt 后台显示。
 */
const char *order_error_to_string(int err)
{
    switch (err) {
    case ORDER_ERR_OK:
        return "OK";

    case ORDER_ERR_INVALID_PARAM:
        return "INVALID_PARAM";

    case ORDER_ERR_MEMBER_NOT_LOGIN:
        return "MEMBER_NOT_LOGIN";

    case ORDER_ERR_PRODUCT_NOT_FOUND:
        return "PRODUCT_NOT_FOUND";

    case ORDER_ERR_OUT_OF_STOCK:
        return "OUT_OF_STOCK";

    case ORDER_ERR_BALANCE_NOT_ENOUGH:
        return "BALANCE_NOT_ENOUGH";

    case ORDER_ERR_PAY_FAILED:
        return "PAY_FAILED";

    case ORDER_ERR_STOCK_DEDUCT_FAILED:
        return "STOCK_DEDUCT_FAILED";

    default:
        return "UNKNOWN";
    }
}
