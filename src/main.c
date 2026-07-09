#include <stdio.h>
#include <string.h>

#include "product/product_manager.h"
#include "member/member_manager.h"
#include "order/order_manager.h"
#include "order/order_state.h"


int main(void)
{
    order_info_t order;
    int ret;

    memset(&order, 0, sizeof(order));

    product_manager_init();
    member_manager_init();
    order_manager_init();
    member_mock_login();

    printf("before buy:\n");
    product_printf_all();

    ret = order_process_buy(1, 5, &order);

    printf("ret=%d\n", ret);
    printf("order_id=%s\n", order.order_id);
    printf("product=%s\n", order.product_name);
    printf("count=%d\n", order.product_count);
    printf("total=%.2f\n", order.total_price);
    printf("balance before=%.2f\n", order.balance_before_pay);
    printf("balance after=%.2f\n", order.balance_after_pay);
    printf("state=%s\n", order_state_to_string(order.state));

    printf("after buy:\n");
    product_printf_all();

    return 0;
}
