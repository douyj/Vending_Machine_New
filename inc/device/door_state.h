#ifndef __DOOR_STATE_H__
#define __DOOR_STATE_H__

#include "log/log.h"

typedef enum {
    DOOR_STATE_CLOSED = 0,     // 柜门已关闭
    DOOR_STATE_OPENING,        // 柜门正在打开
    DOOR_STATE_OPENED,         // 柜门已打开
    DOOR_STATE_CLOSING,        // 柜门正在关闭
    DOOR_STATE_ERROR           // 柜门异常
} door_state_t;


/*
 * @brief 柜门状态转字符串
 *
 * 用于日志打印、MQTT 上报、Qt 后台显示。
 *
 * @param state 柜门状态
 * @return 状态字符串
 */
const char *door_state_to_string(door_state_t state);



#endif
