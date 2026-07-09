#include <stdio.h>
#include <string.h>

#include "product/product_manager.h"
#include "member/member_manager.h"
#include "order/order_manager.h"
#include "order/order_state.h"
#include "device/door_manager.h"
#include "device/door_state.h"
#include "storage/storage_manager.h"

int main(void)
{
    order_info_t order;
    int ret;

    memset(&order, 0, sizeof(order));

       product_manager_init();
       member_manager_init();
       order_manager_init();
       door_manager_init();

       storage_manager_init("vending_machine.db");
       storage_create_tables();

       member_mock_login();
       door_open("member_login_success");

       ret = order_process_buy(1, 5, &order);
       if (ret == ORDER_ERR_OK) {
       storage_insert_order(&order);
       door_close("order_done");
       }

       storage_close();

    return 0;
}