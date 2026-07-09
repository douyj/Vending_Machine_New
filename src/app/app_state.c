#include <stdio.h>

#include "pthread.h"
#include "app/app_state.h"
#include "log/log.h"

static app_state_t g_app_state = APP_STATE_INIT;
static pthread_mutex_t g_state_mutex = PTHREAD_MUTEX_INITIALIZER;

const char *app_state_to_string(app_state_t state)
{
    switch (state) {
    case APP_STATE_INIT:
        return "INIT";
    case APP_STATE_IDLE:
        return "IDLE";
    case APP_STATE_SELECTING:
        return "SELECTING";
    case APP_STATE_PAYING:
        return "PAYING";
    case APP_STATE_DISPENSING:
        return "DISPENSING";
    case APP_STATE_CAPTURING:
        return "CAPTURING";
    case APP_STATE_UPLOADING:
        return "UPLOADING";
    case APP_STATE_SUCCESS:
        return "SUCCESS";
    case APP_STATE_OFFLINE:
        return "OFFLINE";
    case APP_STATE_ERROR:
        return "ERROR";
    case APP_STATE_OTA_UPDATING:
        return "OTA_UPDATING";
    default:
        return "UNKNOWN";
    }
}

/*
    @brief: 初始化应用程序状态
    @return: 无
*/
void app_state_init(void)
{
    pthread_mutex_lock(&g_state_mutex);
    g_app_state = APP_STATE_INIT;
    pthread_mutex_unlock(&g_state_mutex);

    LOG_INFO("app state initialized to %s", app_state_to_string(g_app_state));  
}

/*
    @brief: 获取当前应用程序状态
    @return: 无
*/
app_state_t app_state_get(void)
{
    pthread_mutex_lock(&g_state_mutex);

    app_state_t state = g_app_state;

    pthread_mutex_unlock(&g_state_mutex);

    return state;
}

/*
    @brief: 检查状态转换是否合法
    @param old_state: 当前状态
    @param new_state: 目标状态
    @return: 1表示合法，0表示不合法
*/
int app_state_can_transition(app_state_t old_state, app_state_t new_state)
{
    if(old_state == new_state) {
        return 1; // 允许相同状态的转换
    }

    switch (old_state) {
    case APP_STATE_INIT:
        return new_state == APP_STATE_IDLE ||   
               new_state == APP_STATE_ERROR;

    case APP_STATE_IDLE:
        return new_state == APP_STATE_SELECTING ||
               new_state == APP_STATE_OFFLINE ||
               new_state == APP_STATE_OTA_UPDATING ||
               new_state == APP_STATE_ERROR;

    case APP_STATE_SELECTING:
        return new_state == APP_STATE_PAYING ||
               new_state == APP_STATE_IDLE ||
               new_state == APP_STATE_ERROR;

    case APP_STATE_PAYING:
        return new_state == APP_STATE_DISPENSING ||
               new_state == APP_STATE_IDLE ||
               new_state == APP_STATE_ERROR;

    case APP_STATE_DISPENSING:
        return new_state == APP_STATE_CAPTURING ||
               new_state == APP_STATE_ERROR;

    case APP_STATE_CAPTURING:
        return new_state == APP_STATE_UPLOADING ||
               new_state == APP_STATE_ERROR;

    case APP_STATE_UPLOADING:
        return new_state == APP_STATE_SUCCESS ||
               new_state == APP_STATE_OFFLINE ||
               new_state == APP_STATE_ERROR;

    case APP_STATE_SUCCESS:
        return new_state == APP_STATE_IDLE;

    case APP_STATE_OFFLINE:
        return new_state == APP_STATE_IDLE ||
               new_state == APP_STATE_ERROR;

    case APP_STATE_ERROR:
        return new_state == APP_STATE_IDLE ||
               new_state == APP_STATE_INIT;

    case APP_STATE_OTA_UPDATING:
        return new_state == APP_STATE_IDLE ||
               new_state == APP_STATE_ERROR;

    default:
        return 0;
    }

}

/*
    @brief: 设置应用程序状态
    @param new_state: 目标状态
    @return: 0表示成功，-1表示状态转换不合法
*/
int app_state_set(app_state_t new_state)
{
    pthread_mutex_lock(&g_state_mutex);

    app_state_t old_state = g_app_state;
    g_app_state = new_state;

    if(!app_state_can_transition(old_state, new_state)) {
        LOG_ERROR("Invalid state transition from %s to %s", app_state_to_string(old_state), app_state_to_string(new_state));
        pthread_mutex_unlock(&g_state_mutex);
        return -1; // 状态转换不合法
    }

    pthread_mutex_unlock(&g_state_mutex);
    LOG_INFO("transitioned:%s -> %s", app_state_to_string(old_state), app_state_to_string(new_state));
    return 0;
}
