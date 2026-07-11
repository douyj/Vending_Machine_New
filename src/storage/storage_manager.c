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

    const char *member_sql =
        "CREATE TABLE IF NOT EXISTS members ("
        "member_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT UNIQUE,"
        "password TEXT,"
        "member_name TEXT,"
        "balance REAL,"
        "create_time INTEGER,"
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

    if (sqlite3_exec(g_db, member_sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        LOG_ERROR("create members table failed: %s", errmsg);
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

    if (storage_insert_default_member() != STORAGE_ERR_OK) {
        return STORAGE_ERR_EXEC_FAILED;
    }

    LOG_INFO("storage tables created");
    return STORAGE_ERR_OK;
}

/*
    @brief 插入默认测试会员
    @return 存储错误码
*/
int storage_insert_default_member(void)
{
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "INSERT OR IGNORE INTO members ("
        "username, password, member_name, balance, create_time, update_time"
        ") VALUES (?, ?, ?, ?, strftime('%s','now'), strftime('%s','now'));";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("prepare insert default member failed: %s", sqlite3_errmsg(g_db));
        return STORAGE_ERR_EXEC_FAILED;
    }

    sqlite3_bind_text(stmt, 1, "test", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, "123456", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, "dyj", -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, 1000.0);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        LOG_ERROR("insert default member failed: %s", sqlite3_errmsg(g_db));
        sqlite3_finalize(stmt);
        return STORAGE_ERR_EXEC_FAILED;
    }

    sqlite3_finalize(stmt);
    LOG_INFO("default member ready");
    return STORAGE_ERR_OK;
}

/*
    @brief 根据账号密码查询会员
    @param username 账号
    @param password 密码
    @param out_member 输出会员信息
    @return 存储错误码
*/
int storage_find_member_by_login(const char *username,
                                 const char *password,
                                 member_info_t *out_member)
{
    sqlite3_stmt *stmt = NULL;
    const unsigned char *name = NULL;
    int step_ret;
    const char *sql =
        "SELECT member_id, member_name, balance "
        "FROM members "
        "WHERE username = ? AND password = ?;";

    if (username == NULL || password == NULL || out_member == NULL) {
        return STORAGE_ERR_INVALID_PARAM;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("prepare find member failed: %s", sqlite3_errmsg(g_db));
        return STORAGE_ERR_EXEC_FAILED;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_TRANSIENT);

    step_ret = sqlite3_step(stmt);
    if (step_ret == SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return STORAGE_ERR_NOT_FOUND;
    }

    if (step_ret != SQLITE_ROW) {
        LOG_ERROR("find member failed: %s", sqlite3_errmsg(g_db));
        sqlite3_finalize(stmt);
        return STORAGE_ERR_EXEC_FAILED;
    }

    memset(out_member, 0, sizeof(member_info_t));

    out_member->member_id = sqlite3_column_int(stmt, 0);

    name = sqlite3_column_text(stmt, 1);
    snprintf(out_member->member_name,
             sizeof(out_member->member_name),
             "%s",
             name ? (const char *)name : "");

    out_member->balance = sqlite3_column_double(stmt, 2);
    out_member->logged_in = 1;

    sqlite3_finalize(stmt);
    return STORAGE_ERR_OK;
}

/*
    @brief 更新会员余额
    @param member_id 会员ID
    @param balance 最新余额
    @return 存储错误码
*/
int storage_update_member_balance(int member_id, double balance)
{
    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "UPDATE members "
        "SET balance = ?, update_time = strftime('%s','now') "
        "WHERE member_id = ?;";

    if (member_id <= 0 || balance < 0.0) {
        return STORAGE_ERR_INVALID_PARAM;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("prepare update member balance failed: %s", sqlite3_errmsg(g_db));
        return STORAGE_ERR_EXEC_FAILED;
    }

    sqlite3_bind_double(stmt, 1, balance);
    sqlite3_bind_int(stmt, 2, member_id);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        LOG_ERROR("update member balance failed: %s", sqlite3_errmsg(g_db));
        sqlite3_finalize(stmt);
        return STORAGE_ERR_EXEC_FAILED;
    }

    if (sqlite3_changes(g_db) <= 0) {
        sqlite3_finalize(stmt);
        return STORAGE_ERR_NOT_FOUND;
    }

    sqlite3_finalize(stmt);
    LOG_INFO("update member balance success, member_id=%d, balance=%.2f",
             member_id,
             balance);
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
    @brief 获取下一个订单序号
    @return 下一个订单序号，失败时返回 1
*/
int storage_get_next_order_seq(void)
{
    sqlite3_stmt *stmt = NULL;
    int next_seq = 1;
    const char *sql =
        "SELECT COALESCE(MAX(CAST(SUBSTR(order_id, 7) AS INTEGER)), 0) + 1 "
        "FROM orders "
        "WHERE order_id LIKE 'ORDER_%';";

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("prepare get next order seq failed: %s", sqlite3_errmsg(g_db));
        return next_seq;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        next_seq = sqlite3_column_int(stmt, 0);
        if (next_seq <= 0) {
            next_seq = 1;
        }
    }

    sqlite3_finalize(stmt);
    return next_seq;
}

/*
    @brief 加载最近订单
    @param out_orders 输出订单数组
    @param max_count 最大订单数
    @return 存储错误码
    @note 从订单表中加载最近订单
*/
int storage_load_recent_orders(order_info_t *out_orders, int max_count)
{
    sqlite3_stmt *stmt = NULL;
    const unsigned char *text = NULL;
    int count = 0;

    const char *sql =
        "SELECT order_id, member_id, member_name, product_id, product_name, "
        "product_count, unit_price, total_price, "
        "balance_before_pay, balance_after_pay, "
        "order_state, upload_status, create_time "
        "FROM orders "
        "ORDER BY create_time DESC, id DESC "
        "LIMIT ?;";

    if (out_orders == NULL || max_count <= 0) {
        return STORAGE_ERR_INVALID_PARAM;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        LOG_ERROR("prepare load recent orders failed: %s", sqlite3_errmsg(g_db));
        return STORAGE_ERR_EXEC_FAILED;
    }

    sqlite3_bind_int(stmt, 1, max_count);

    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_count) {
        order_info_t *order = &out_orders[count];

        memset(order, 0, sizeof(order_info_t));

        text = sqlite3_column_text(stmt, 0);
        snprintf(order->order_id, sizeof(order->order_id), "%s",
                 text ? (const char *)text : "");

        order->member_id = sqlite3_column_int(stmt, 1);

        text = sqlite3_column_text(stmt, 2);
        snprintf(order->member_name, sizeof(order->member_name), "%s",
                 text ? (const char *)text : "");

        order->product_id = sqlite3_column_int(stmt, 3);

        text = sqlite3_column_text(stmt, 4);
        snprintf(order->product_name, sizeof(order->product_name), "%s",
                 text ? (const char *)text : "");

        order->product_count = sqlite3_column_int(stmt, 5);
        order->unit_price = sqlite3_column_double(stmt, 6);
        order->total_price = sqlite3_column_double(stmt, 7);
        order->balance_before_pay = sqlite3_column_double(stmt, 8);
        order->balance_after_pay = sqlite3_column_double(stmt, 9);

        /*
         * 这里先简单处理：数据库里存的是状态字符串。
         * UI 查询时主要显示文字，不强依赖 enum。
         * 如果后面要严格恢复 enum，可以写 order_state_from_string()。
         */
        order->state = ORDER_STATE_DONE;

        order->upload_status = sqlite3_column_int(stmt, 11);
        order->create_time = sqlite3_column_int64(stmt, 12);

        count++;
    }

    sqlite3_finalize(stmt);

    return count;
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






/*
    @brief 存储错误码转字符串
    @param err 错误码
    @return 错误码字符串
*/
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
