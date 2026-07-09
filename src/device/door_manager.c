#include <stdio.h>
#include <pthread.h>

#include "log/log.h"
#include "device/door_manager.h"
#include "device/door_state.h"

static door_state_t g_door_state = DOOR_STATE_CLOSED;
static pthread_mutex_t g_door_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
    @brief 初始化柜门管理器
    @return DOOR_ERR_OK 成功
    @return 其他 错误码
*/
int door_manager_init(void)
{
    pthread_mutex_lock(&g_door_mutex);
    g_door_state = DOOR_STATE_CLOSED;
    pthread_mutex_unlock(&g_door_mutex);

    LOG_INFO("door manager init, state %s", door_state_to_string(g_door_state));
    return DOOR_ERR_OK;
}

/*
    @brief 获取柜门状态
    @return 柜门状态
*/
door_state_t door_get_state(void)
{
    door_state_t state;
    pthread_mutex_lock(&g_door_mutex);
    state = g_door_state;
    pthread_mutex_unlock(&g_door_mutex);
    return state;   
}

/*
    @brief 检查柜门是否已打开
    @return 1 已打开
    @return 0 未打开
*/
int door_is_opened(void)
{
    return door_get_state() == DOOR_STATE_OPENED;
}

/*
    @brief 检查柜门是否已关闭   
    @return 1 已关闭
    @return 0 未关闭
*/
int door_is_closed(void)
{
    return door_get_state() == DOOR_STATE_CLOSED;
}


int door_open(const char *reason)
{
    pthread_mutex_lock(&g_door_mutex);

    if (g_door_state == DOOR_STATE_OPENED) {
        LOG_INFO("door already opened, reason=%s",
                 reason ? reason : "none");
        pthread_mutex_unlock(&g_door_mutex);
        return DOOR_ERR_OK;
    }

    if (g_door_state == DOOR_STATE_CLOSING) {
        LOG_WARN("door open failed, door is closing");
        pthread_mutex_unlock(&g_door_mutex);
        return DOOR_ERR_INVALID_STATE;
    }

    g_door_state = DOOR_STATE_OPENING;
    LOG_INFO("door opening mock, reason=%s",
             reason ? reason : "none");

    /*
     * 第一版不接舵机，所以直接认为开门完成。
     * 后面接 PWM 时，这里替换成 servo_open()。
     */
    g_door_state = DOOR_STATE_OPENED;
    LOG_INFO("door opened mock, state=%s",
             door_state_to_string(g_door_state));

    pthread_mutex_unlock(&g_door_mutex);
    return DOOR_ERR_OK;
}


int door_close(const char *reason)
{
    pthread_mutex_lock(&g_door_mutex);

    if (g_door_state == DOOR_STATE_CLOSED) {
        LOG_INFO("door already closed, reason=%s",
                 reason ? reason : "none");
        pthread_mutex_unlock(&g_door_mutex);
        return DOOR_ERR_OK;
    }

    if (g_door_state == DOOR_STATE_OPENING) {
        LOG_WARN("door close failed, door is opening");
        pthread_mutex_unlock(&g_door_mutex);
        return DOOR_ERR_INVALID_STATE;
    }

    g_door_state = DOOR_STATE_CLOSING;
    LOG_INFO("door closing mock, reason=%s",
             reason ? reason : "none");

    /*
     * 第一版不接舵机，所以直接认为关门完成。
     * 后面接 PWM 时，这里替换成 servo_close()。
     */
    g_door_state = DOOR_STATE_CLOSED;
    LOG_INFO("door closed mock, state=%s",
             door_state_to_string(g_door_state));

    pthread_mutex_unlock(&g_door_mutex);
    return DOOR_ERR_OK;
}

const char *door_error_to_string(int err)
{
    switch (err) {
    case DOOR_ERR_OK:
        return "OK";
    case DOOR_ERR_INVALID_PARAM:
        return "INVALID_PARAM";
    case DOOR_ERR_INVALID_STATE:
        return "INVALID_STATE";
    case DOOR_ERR_OPEN_FAILED:
        return "OPEN_FAILED";
    case DOOR_ERR_CLOSE_FAILED:
        return "CLOSE_FAILED";
    default:
        return "UNKNOWN";
    }
}