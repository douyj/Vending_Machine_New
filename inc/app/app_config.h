#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#define APP_CONFIG_DEVICE_ID_MAX_LEN 32
#define APP_CONFIG_DEVICE_NAME_MAX_LEN 64
#define APP_CONFIG_HOST_MAX_LEN 64
#define APP_CONFIG_DB_PATH_MAX_LEN 128

typedef struct {
    char device_id[APP_CONFIG_DEVICE_ID_MAX_LEN];
    char device_name[APP_CONFIG_DEVICE_NAME_MAX_LEN];
    char tcp_host[APP_CONFIG_HOST_MAX_LEN];
    int tcp_port;
    char db_path[APP_CONFIG_DB_PATH_MAX_LEN];
} app_config_t;

/*
 * @brief 加载应用配置
 *
 * 如果配置文件不存在或字段缺失，会使用默认值继续运行。
 *
 * @param path 配置文件路径，例如 "config/device_config.json"
 * @return 0 成功，-1 使用默认配置或解析失败
 */
int app_config_load(const char *path);

/*
 * @brief 获取当前应用配置
 */
const app_config_t *app_config_get(void);

/*
 * @brief 获取当前设备 ID
 */
const char *app_config_get_device_id(void);

#endif
