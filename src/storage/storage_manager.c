#include <string.h>
#include <stdio.h>

#include "log/log.h"
#include "storage/storage_manager.h"
#include "sqlite3.h"


static sqlite3 *g_db = NULL;

/*
    @brief 初始化存储管理器
    @param db_path 数据库路径
    @return 存储错误码
    @note 初始化数据库连接，创建表等
*/
int storage_manager_init(const char *db_path)
{
    if (db_path == NULL) {
        LOG_ERROR("db_path is NULL");
        return STORAGE_ERR_INVALID_PARAM;
    }

    if(sqlite3_open(db_path, &g_db) != SQLITE_OK) {
        LOG_ERROR("open db failed: %s", sqlite3_errmsg(g_db));
        return STORAGE_ERR_OPEN_FAILED;
    }

    LOG_INFO("storage opened: %s", db_path);
    return STORAGE_ERR_OK;
}

/*
    @brief 关闭存储管理器
    @return 存储错误码
    @note 关闭数据库连接，释放资源
*/
int storage_close(void)
{
    if(g_db != NULL){
        sqlite3_close(g_db);
        g_db = NULL;
    }

    LOG_INFO("storage closed");
    return STORAGE_ERR_OK;
}


/*
    @brief 创建数据库表
    @return 存储错误码
    @note 创建订单表等
*/
int storage_create_tables(void)
{
    char *errmsg = NULL;

    const char *sql = 
        "CREATE TABLE IF NOT EXISTS orders ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "order_id TEXT UNIQUE,"
        "member_id INTEGER,"
        "member_name TEXT,"
        "product_id INTEGER,"
        "product_name TEXT,"
        "product_count INTEGER,"
        "unit_price REAL,"
        "total_price REAL,"
        "balance_before_pay REAL,"
        "balance_after_pay REAL,"
        "order_state TEXT,"
        "upload_status INTEGER,"
        "create_time INTEGER"
        ");";

    if (sqlite3_exec(g_db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        LOG_ERROR("create table failed: %s", errmsg);
        sqlite3_free(errmsg);
        return STORAGE_ERR_EXEC_FAILED;
    }

    return STORAGE_ERR_OK;
}


/*
    @brief 插入订单到数据库
    @param order 订单信息
    @return 存储错误码
    @note 插入订单到订单表中
*/
int storage_insert_order(const order_info_t *order)
{
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "INSERT INTO orders ("
        "order_id, member_id, member_name, product_id, product_name,"
        "product_count, unit_price, total_price,"
        "balance_before_pay, balance_after_pay,"
        "order_state, upload_status, create_time"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    if(order == NULL){
        return STORAGE_ERR_INVALID_PARAM;
    }

    if(sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("prepare insert order failed: %s", sqlite3_errmsg(g_db));
        return STORAGE_ERR_EXEC_FAILED;
    }

    sqlite3_bind_text(stmt, 1, order->order_id, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, order->member_id);
    sqlite3_bind_text(stmt, 3, order->member_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, order->product_id);
    sqlite3_bind_text(stmt, 5, order->product_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, order->product_count);
    sqlite3_bind_double(stmt, 7, order->unit_price);
    sqlite3_bind_double(stmt, 8, order->total_price);
    sqlite3_bind_double(stmt, 9, order->balance_before_pay);
    sqlite3_bind_double(stmt, 10, order->balance_after_pay);
    sqlite3_bind_text(stmt, 11, order_state_to_string(order->state), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 12, order->upload_status);
    sqlite3_bind_int64(stmt, 13, order->create_time);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        LOG_ERROR("insert order failed: %s", sqlite3_errmsg(g_db));
        sqlite3_finalize(stmt);
        return STORAGE_ERR_EXEC_FAILED;
    }

    sqlite3_finalize(stmt);

    LOG_INFO("insert order success, order_id=%s", order->order_id);
    return STORAGE_ERR_OK;
}

