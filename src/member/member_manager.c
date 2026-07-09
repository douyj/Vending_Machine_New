#include "member/member_manager.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>

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
    @brief 模拟会员登录
    @return MEMBER_ERR_OK 成功
*/
int member_mock_login(void)
{
    pthread_mutex_lock(&g_member_mutex);

    g_current_member.member_id = 10001;
    snprintf(g_current_member.member_name,
             sizeof(g_current_member.member_name),
             "%s",
             "dyj");
    g_current_member.balance = 100.0;
    g_current_member.logged_in = 1;

    pthread_mutex_unlock(&g_member_mutex);

    LOG_INFO("member mock login success, member_id=%d, name=%s, balance=%.2f",
             10001,
             "dyj",
             100.0);
    return MEMBER_ERR_OK;
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

    g_current_member.balance -= amount;

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
    default:
        return "UNKNOWN";
    }
}
