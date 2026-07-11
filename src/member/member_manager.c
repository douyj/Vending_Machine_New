#include "member/member_manager.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "storage/storage_manager.h"

static member_info_t g_current_member;
static pthread_mutex_t g_member_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
    @brief 初始化会员管理器
    @return MEMBER_ERR_OK 成功
*/
int member_manager_init(void)
{
    pthread_mutex_lock(&g_member_mutex);
    memset(&g_current_member, 0, sizeof(g_current_member));
    pthread_mutex_unlock(&g_member_mutex);

    LOG_INFO("member manager initialized");
    return MEMBER_ERR_OK;
}

/*
    @brief 会员登录
    @param username 账号
    @param password 密码
    @return MEMBER_ERR_OK 成功，其他失败
*/
int member_login(const char *username, const char *password)
{
    member_info_t member;
    int ret;

    if (username == NULL || password == NULL) {
        LOG_WARN("member login invalid param");
        return MEMBER_ERR_INVALID_PARAM;
    }

    ret = storage_find_member_by_login(username, password, &member);
    if (ret == STORAGE_ERR_NOT_FOUND) {
        LOG_WARN("member login failed, not found, username=%s", username);
        return MEMBER_ERR_NOT_FOUND;
    }

    if (ret != STORAGE_ERR_OK) {
        LOG_WARN("member login failed, storage ret=%d", ret);
        return MEMBER_ERR_STORAGE_FAILED;
    }

    pthread_mutex_lock(&g_member_mutex);
    g_current_member = member;
    pthread_mutex_unlock(&g_member_mutex);

    LOG_INFO("member login success, member_id=%d, name=%s, balance=%.2f",
             member.member_id,
             member.member_name,
             member.balance);

    return MEMBER_ERR_OK;
}

/*
    @brief 模拟会员登录
    @return MEMBER_ERR_OK 成功
*/
int member_mock_login(void)
{
    return member_login("test", "123456");
}

/*
    @brief 会员退出登录
    @return MEMBER_ERR_OK 成功
*/
int member_logout(void)
{
    pthread_mutex_lock(&g_member_mutex);
    memset(&g_current_member, 0, sizeof(g_current_member));
    pthread_mutex_unlock(&g_member_mutex);

    LOG_INFO("member logout success");
    return MEMBER_ERR_OK;
}

/*
    @brief 判断当前是否有会员登录
    @return 1 已登录，0 未登录
*/
int member_is_logged_in(void)
{
    int logged_in;

    pthread_mutex_lock(&g_member_mutex);
    logged_in = g_current_member.logged_in;
    pthread_mutex_unlock(&g_member_mutex);

    return logged_in;
}

/*
    @brief 获取当前会员信息
    @param out_member 输出会员信息
    @return MEMBER_ERR_OK 成功，其他失败
*/
int member_get_current(member_info_t *out_member)
{
    if (out_member == NULL) {
        LOG_WARN("member get current invalid param");
        return MEMBER_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&g_member_mutex);

    if (!g_current_member.logged_in) {
        pthread_mutex_unlock(&g_member_mutex);
        LOG_WARN("member get current failed, not login");
        return MEMBER_ERR_NOT_LOGIN;
    }

    *out_member = g_current_member;

    pthread_mutex_unlock(&g_member_mutex);
    return MEMBER_ERR_OK;
}

/*
    @brief 检查当前会员余额是否足够
    @param amount 需要支付的金额
    @return MEMBER_ERR_OK 余额足够，其他失败
*/
int member_check_balance(double amount)
{
    if (amount <= 0) {
        LOG_WARN("member check balance invalid amount=%.2f", amount);
        return MEMBER_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&g_member_mutex);

    if (!g_current_member.logged_in) {
        pthread_mutex_unlock(&g_member_mutex);
        LOG_WARN("member check balance failed, not login");
        return MEMBER_ERR_NOT_LOGIN;
    }

    if (g_current_member.balance < amount) {
        LOG_WARN("member check balance failed, balance=%.2f, need=%.2f",
                 g_current_member.balance,
                 amount);
        pthread_mutex_unlock(&g_member_mutex);
        return MEMBER_ERR_BALANCE_NOT_ENOUGH;
    }

    pthread_mutex_unlock(&g_member_mutex);
    return MEMBER_ERR_OK;
}

/*
    @brief 扣减当前会员余额
    @param amount 扣款金额
    @param before 输出扣款前余额，可为 NULL
    @param after 输出扣款后余额，可为 NULL
    @return MEMBER_ERR_OK 成功，其他失败
*/
int member_deduct_balance(double amount, double *before, double *after)
{
    double old_balance;
    double new_balance;
    int ret;

    if (amount <= 0) {
        LOG_WARN("member deduct balance invalid amount=%.2f", amount);
        return MEMBER_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&g_member_mutex);

    if (!g_current_member.logged_in) {
        pthread_mutex_unlock(&g_member_mutex);
        LOG_WARN("member deduct balance failed, not login");
        return MEMBER_ERR_NOT_LOGIN;
    }

    if (g_current_member.balance < amount) {
        LOG_WARN("member deduct balance failed, balance=%.2f, need=%.2f",
                 g_current_member.balance,
                 amount);
        pthread_mutex_unlock(&g_member_mutex);
        return MEMBER_ERR_BALANCE_NOT_ENOUGH;
    }

    if (before != NULL) {
        *before = g_current_member.balance;
    }

    old_balance = g_current_member.balance;
    new_balance = old_balance - amount;

    ret = storage_update_member_balance(g_current_member.member_id, new_balance);
    if (ret != STORAGE_ERR_OK) {
        pthread_mutex_unlock(&g_member_mutex);
        LOG_WARN("member deduct balance failed, update storage failed, member_id=%d, ret=%d",
                 g_current_member.member_id,
                 ret);
        return MEMBER_ERR_STORAGE_FAILED;
    }

    g_current_member.balance = new_balance;

    if (after != NULL) {
        *after = g_current_member.balance;
    }

    LOG_INFO("member deduct balance success, member_id=%d, amount=%.2f, balance=%.2f",
             g_current_member.member_id,
             amount,
             g_current_member.balance);

    pthread_mutex_unlock(&g_member_mutex);
    return MEMBER_ERR_OK;
}

/*
    @brief 查询当前会员余额
    @param out_balance 输出余额
    @return MEMBER_ERR_OK 成功，其他失败
*/
int member_get_balance(double *out_balance)
{
    if (out_balance == NULL) {
        LOG_WARN("member get balance invalid param");
        return MEMBER_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&g_member_mutex);

    if (!g_current_member.logged_in) {
        pthread_mutex_unlock(&g_member_mutex);
        LOG_WARN("member get balance failed, not login");
        return MEMBER_ERR_NOT_LOGIN;
    }

    *out_balance = g_current_member.balance;

    pthread_mutex_unlock(&g_member_mutex);
    return MEMBER_ERR_OK;
}

/*
    @brief 会员错误码转字符串
    @param err 错误码
    @return 错误码字符串
*/
const char *member_error_to_string(int err)
{
    switch (err) {
    case MEMBER_ERR_OK:
        return "OK";
    case MEMBER_ERR_INVALID_PARAM:
        return "INVALID_PARAM";
    case MEMBER_ERR_NOT_LOGIN:
        return "NOT_LOGIN";
    case MEMBER_ERR_BALANCE_NOT_ENOUGH:
        return "BALANCE_NOT_ENOUGH";
    case MEMBER_ERR_ALREADY_LOGIN:
        return "ALREADY_LOGIN";
    case MEMBER_ERR_NOT_FOUND:
        return "NOT_FOUND";
    case MEMBER_ERR_STORAGE_FAILED:
        return "STORAGE_FAILED";
    default:
        return "UNKNOWN";
    }
}
