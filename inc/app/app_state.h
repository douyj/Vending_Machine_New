#ifndef APP_STATE_H
#define APP_STATE_H

/*
开机
  ↓
初始化
  ↓
空闲等待用户
  ↓
用户选择商品
  ↓
支付
  ↓
出货
  ↓
抓拍
  ↓
上传订单
  ↓
完成
  ↓
回到空闲
*/

typedef enum {
    APP_STATE_INIT = 0,     // 初始化
    APP_STATE_IDLE,         // 空闲
    APP_STATE_SELECTING,    // 选择中
    APP_STATE_PAYING,       // 支付中
    APP_STATE_DISPENSING,   // 出货中
    APP_STATE_CAPTURING,    // 拍照中
    APP_STATE_UPLOADING,    // 上传中
    APP_STATE_SUCCESS,      // 成功
    APP_STATE_ERROR,        // 错误
    APP_STATE_OFFLINE,      // 离线
    APP_STATE_OTA_UPDATING  // OTA升级中
} app_state_t;

const char *app_state_to_string(app_state_t state);
void app_state_init(void);
app_state_t app_state_get(void);
int app_state_can_transition(app_state_t old_state, app_state_t new_state);
int app_state_set(app_state_t new_state);

#endif // APP_STATE_H