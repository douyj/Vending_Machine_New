#ifndef __MEMBER_MANAGER_H__
#define __MEMBER_MANAGER_H__

#include "log/log.h"

#define MEMBER_NAME_MAX_LEN 20

/*
    @brief 会员错误码
*/
typedef enum {
    MEMBER_ERR_OK = 0,
    MEMBER_ERR_INVALID_PARAM = -1,
    MEMBER_ERR_NOT_LOGIN = -2,
    MEMBER_ERR_BALANCE_NOT_ENOUGH = -3,
    MEMBER_ERR_ALREADY_LOGIN = -4,
    MEMBER_ERR_NOT_FOUND = -5,
    MEMBER_ERR_STORAGE_FAILED = -6,
    MEMBER_ERR_ALREADY_EXISTS = -7
} member_err_t;


typedef struct {
    int member_id;                              // 会员ID
    char member_name[MEMBER_NAME_MAX_LEN];      // 会员姓名
    double balance;                             // 会员余额
    int logged_in;                              // 是否已登录，1=已登录，0=未登录
} member_info_t;

int member_manager_init(void);
int member_login(const char *username, const char *password);
int member_register(const char *username,
                    const char *password,
                    const char *member_name);
int member_mock_login(void);
int member_logout(void);
int member_is_logged_in(void);
int member_get_current(member_info_t *out_member);
int member_check_balance(double amount);
int member_deduct_balance(double amount, double *before, double *after);
int member_get_balance(double *out_balance);
const char *member_error_to_string(int err);


#endif
