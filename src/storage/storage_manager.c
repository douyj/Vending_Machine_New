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

    /* 创建订单表 */
    const char *order_sql =
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

    const char *product_sql =
        "CREATE TABLE IF NOT EXISTS products ("
        "product_id INTEGER PRIMARY KEY,"
        "product_name TEXT,"
        "product_price REAL,"
        "product_stock INTEGER,"
        "product_category INTEGER,"
        "update_time INTEGER"
        ");";
    const char *product_category_migration_sql =
        "ALTER TABLE products ADD COLUMN product_category INTEGER DEFAULT 4;";

    if (sqlite3_exec(g_db, order_sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        LOG_ERROR("create orders table failed: %s", errmsg);
        sqlite3_free(errmsg);
        return STORAGE_ERR_EXEC_FAILED;
    }

    if (sqlite3_exec(g_db, product_sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        LOG_ERROR("create products table failed: %s", errmsg);
        sqlite3_free(errmsg);
        return STORAGE_ERR_EXEC_FAILED;
    }

    if (sqlite3_exec(g_db, product_category_migration_sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        if (errmsg == NULL || strstr(errmsg, "duplicate column name") == NULL) {
            LOG_ERROR("migrate products table failed: %s", errmsg);
            sqlite3_free(errmsg);
            return STORAGE_ERR_EXEC_FAILED;
        }
        sqlite3_free(errmsg);
        errmsg = NULL;
    }

    LOG_INFO("storage tables created");
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

/*
    @brief 插入或更新商品信息
    @param product 商品信息
    @return 存储错误码
*/
int storage_insert_or_update_product(const product_info_t *product)
{
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "INSERT OR REPLACE INTO products ("
        "product_id, product_name, product_price, product_stock, product_category, update_time"
        ") VALUES (?, ?, ?, ?, ?, strftime('%s','now'));";

    if (product == NULL) {
        return STORAGE_ERR_INVALID_PARAM;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("prepare insert/update product failed: %s", sqlite3_errmsg(g_db));
        return STORAGE_ERR_EXEC_FAILED;
    }

    sqlite3_bind_int(stmt, 1, product->product_id);
    sqlite3_bind_text(stmt, 2, product->product_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, product->product_price);
    sqlite3_bind_int(stmt, 4, product->product_stock);
    sqlite3_bind_int(stmt, 5, product->product_category);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        LOG_ERROR("insert/update product failed: %s", sqlite3_errmsg(g_db));
        sqlite3_finalize(stmt);
        return STORAGE_ERR_EXEC_FAILED;
    }

    sqlite3_finalize(stmt);

    LOG_INFO("insert/update product success, product_id=%d, stock=%d",
             product->product_id,
             product->product_stock);
    return STORAGE_ERR_OK;
}

/*
    @brief 更新商品库存
    @param product_id 商品ID
    @param stock 最新库存
    @return 存储错误码
*/
int storage_update_product_stock(int product_id, int stock)
{
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "UPDATE products "
        "SET product_stock = ?, update_time = strftime('%s','now') "
        "WHERE product_id = ?;";

    if (product_id <= 0 || stock < 0) {
        return STORAGE_ERR_INVALID_PARAM;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("prepare update product stock failed: %s", sqlite3_errmsg(g_db));
        return STORAGE_ERR_EXEC_FAILED;
    }

    sqlite3_bind_int(stmt, 1, stock);
    sqlite3_bind_int(stmt, 2, product_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        LOG_ERROR("update product stock failed: %s", sqlite3_errmsg(g_db));
        sqlite3_finalize(stmt);
        return STORAGE_ERR_EXEC_FAILED;
    }

    sqlite3_finalize(stmt);

    LOG_INFO("update product stock success, product_id=%d, stock=%d",
             product_id,
             stock);
    return STORAGE_ERR_OK;
}

/*
    @brief 从数据库读取一个商品
    @param product_id 商品ID
    @param out_product 输出商品信息
    @return 存储错误码
*/
int storage_load_product(int product_id, product_info_t *out_product)
{
    sqlite3_stmt *stmt = NULL;
    const unsigned char *name = NULL;
    int step_ret;
    const char *sql =
        "SELECT product_id, product_name, product_price, product_stock, product_category "
        "FROM products WHERE product_id = ?;";

    if (product_id <= 0 || out_product == NULL) {
        return STORAGE_ERR_INVALID_PARAM;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("prepare load product failed: %s", sqlite3_errmsg(g_db));
        return STORAGE_ERR_EXEC_FAILED;
    }

    sqlite3_bind_int(stmt, 1, product_id);

    step_ret = sqlite3_step(stmt);

    if (step_ret == SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return STORAGE_ERR_NOT_FOUND;
    }

    if (step_ret != SQLITE_ROW) {
        LOG_ERROR("load product failed: %s", sqlite3_errmsg(g_db));
        sqlite3_finalize(stmt);
        return STORAGE_ERR_EXEC_FAILED;
    }

    memset(out_product, 0, sizeof(product_info_t));

    out_product->product_id = sqlite3_column_int(stmt, 0);

    name = sqlite3_column_text(stmt, 1);
    snprintf(out_product->product_name,
             sizeof(out_product->product_name),
             "%s",
             name ? (const char *)name : "");

    out_product->product_price = sqlite3_column_double(stmt, 2);
    out_product->product_stock = sqlite3_column_int(stmt, 3);
    out_product->product_category = (product_category_t)sqlite3_column_int(stmt, 4);

    sqlite3_finalize(stmt);
    return STORAGE_ERR_OK;
}

const char *storage_error_to_string(int err)
{
    switch (err) {
    case STORAGE_ERR_OK:
        return "OK";
    case STORAGE_ERR_INVALID_PARAM:
        return "INVALID_PARAM";
    case STORAGE_ERR_OPEN_FAILED:
        return "OPEN_FAILED";
    case STORAGE_ERR_EXEC_FAILED:
        return "EXEC_FAILED";
    case STORAGE_ERR_NOT_FOUND:
        return "NOT_FOUND";
    default:
        return "UNKNOWN";
    }
}
