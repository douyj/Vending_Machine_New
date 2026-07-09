#ifndef __STORAGE_MANAGER_H__
#define __STORAGE_MANAGER_H__

#include "order/order_manager.h"
#include "member/member_manager.h"
#include "product/product_manager.h"

typedef enum {
    STORAGE_ERR_OK = 0,
    STORAGE_ERR_INVALID_PARAM = -1,
    STORAGE_ERR_OPEN_FAILED = -2,
    STORAGE_ERR_EXEC_FAILED = -3
} storage_err_t;


int storage_manager_init(const char *db_path);
int storage_create_tables(void);
int storage_close(void);

int storage_insert_order(const order_info_t *order);

const char *storage_error_to_string(int err);

#endif
