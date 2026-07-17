#ifndef __STORAGE_MANAGER_H__
#define __STORAGE_MANAGER_H__

#include "order/order_manager.h"
#include "member/member_manager.h"
#include "product/product_manager.h"

typedef enum {
    STORAGE_ERR_OK = 0,
    STORAGE_ERR_INVALID_PARAM = -1,
    STORAGE_ERR_OPEN_FAILED = -2,
    STORAGE_ERR_EXEC_FAILED = -3,
    STORAGE_ERR_NOT_FOUND = -4,
    STORAGE_ERR_ALREADY_EXISTS = -5
} storage_err_t;


int storage_manager_init(const char *db_path);
int storage_create_tables(void);
int storage_close(void);

int storage_insert_default_member(void);
int storage_insert_member(const char *username,
                          const char *password,
                          const char *member_name,
                          double balance);
int storage_find_member_by_login(const char *username,
                                 const char *password,
                                 member_info_t *out_member);
int storage_update_member_balance(int member_id, double balance);

int storage_insert_order(const order_info_t *order);
int storage_get_next_order_seq(void);
int storage_insert_or_update_product(const product_info_t *product);
int storage_update_product_stock(int product_id, int stock);
int storage_load_product(int product_id, product_info_t *out_product);

//从数据库中加载最近的订单
int storage_load_recent_orders(order_info_t *out_orders, int max_count);

const char *storage_error_to_string(int err);

#endif
