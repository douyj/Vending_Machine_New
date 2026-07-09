#include <stdio.h>

#include "order/order_state.h"

/*
    @brief 订单状态转换为字符串
    @param state 订单状态
    @return 订单状态字符串
*/
const char *order_state_to_string(order_state_t state)
{
    switch (state) {
    case ORDER_STATE_NONE:
        return "NONE";

    case ORDER_STATE_CREATED:
        return "CREATED";

    case ORDER_STATE_CHECKING_BALANCE:
        return "CHECKING_BALANCE";

    case ORDER_STATE_PAYING:
        return "PAYING";

    case ORDER_STATE_PAID:
        return "PAID";

    case ORDER_STATE_STOCK_DEDUCTED:
        return "STOCK_DEDUCTED";

    case ORDER_STATE_DONE:
        return "DONE";

    case ORDER_STATE_FAILED:
        return "FAILED";

    case ORDER_STATE_PENDING_UPLOAD:
        return "PENDING_UPLOAD";

    default:
        return "UNKNOWN";
    }
}
