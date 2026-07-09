#ifndef __ORDER_STATE_H__
#define __ORDER_STATE_H__

typedef enum {
    ORDER_STATE_NONE = 0,           // 当前没有订单
    ORDER_STATE_CREATED,            // 订单已创建
    ORDER_STATE_CHECKING_BALANCE,   // 检查余额中
    ORDER_STATE_PAYING,             // 支付中/模拟支付中
    ORDER_STATE_PAID,               // 已支付
    ORDER_STATE_STOCK_DEDUCTED,     // 库存已扣减
    ORDER_STATE_DONE,               // 订单完成
    ORDER_STATE_FAILED,             // 订单失败
    ORDER_STATE_PENDING_UPLOAD      // 待上传，后面断网缓存用
} order_state_t;


const char *order_state_to_string(order_state_t state);


#endif
