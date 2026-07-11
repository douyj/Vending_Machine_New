#ifndef __ORDER_MANAGER_H__
#define __ORDER_MANAGER_H__

#include "order/order_state.h"
#include "product/product_manager.h"

#define ORDER_ID_MAX_LEN 32
#define ORDER_PRODUCT_NAME_MAX_LEN 50

typedef enum {
    ORDER_ERR_OK = 0,                       // 成功
    ORDER_ERR_INVALID_PARAM = -1,           // 无效参数
    ORDER_ERR_PRODUCT_NOT_FOUND = -2,
    ORDER_ERR_OUT_OF_STOCK = -3,
    ORDER_ERR_MEMBER_NOT_LOGIN = -4,
    ORDER_ERR_BALANCE_NOT_ENOUGH = -5,
    ORDER_ERR_PAY_FAILED = -6,
    ORDER_ERR_STOCK_DEDUCT_FAILED = -7
} order_error_t;

/*
 * 订单信息结构体
*/
typedef struct{

    char order_id[ORDER_ID_MAX_LEN];

    int member_id;
    char member_name[32];

    int product_id;
    char product_name[ORDER_PRODUCT_NAME_MAX_LEN];

    int product_count;
    double unit_price;      // 单价
    double total_price;     // 总价

    double balance_before_pay; // 支付前余额
    double balance_after_pay;  // 支付后余额

    order_state_t state;
    int upload_status; // 上传状态 0: 未上传 1: 已上传
    long create_time; // 创建时间

}order_info_t;

/*
    @brief 购物车商品结构体 
    @param product_id 商品ID
    @param quantity 商品数量
*/
typedef struct {
    int product_id;
    int quantity;
} order_cart_item_t;

int order_manager_init(void);

int order_create(int product_id, int count, order_info_t *out_order);
int order_check_balance(order_info_t *order);
int order_pay_by_member_balance(order_info_t *order);
int order_deduct_stock(order_info_t *order);
int order_finish(order_info_t *order);

int order_process_buy(int product_id, int count, order_info_t *out_order);
int order_process_cart_buy(const order_cart_item_t *items,
                           int item_count,
                           double *out_paid_total,
                           double *out_balance_after);

const char *order_error_to_string(int err);


#endif  
