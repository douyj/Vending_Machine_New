#include "app/app_config.h"

#include "log/log.h"
#include "third_party/cJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define APP_CONFIG_DEFAULT_DEVICE_ID "VM001"
#define APP_CONFIG_DEFAULT_DEVICE_NAME "Vending Machine 001"
#define APP_CONFIG_DEFAULT_TCP_HOST "127.0.0.1"
#define APP_CONFIG_DEFAULT_TCP_PORT 9000
#define APP_CONFIG_DEFAULT_DB_PATH "vending_machine.db"

static app_config_t g_config;

/*
 * @brief 写入默认配置
 */
static void set_default_config(void)
{
    memset(&g_config, 0, sizeof(g_config));
    snprintf(g_config.device_id, sizeof(g_config.device_id), "%s",
             APP_CONFIG_DEFAULT_DEVICE_ID);
    snprintf(g_config.device_name, sizeof(g_config.device_name), "%s",
             APP_CONFIG_DEFAULT_DEVICE_NAME);
    snprintf(g_config.tcp_host, sizeof(g_config.tcp_host), "%s",
             APP_CONFIG_DEFAULT_TCP_HOST);
    g_config.tcp_port = APP_CONFIG_DEFAULT_TCP_PORT;
    snprintf(g_config.db_path, sizeof(g_config.db_path), "%s",
             APP_CONFIG_DEFAULT_DB_PATH);
}

/*
 * @brief 读取整个文本文件
 */
static char *read_text_file(const char *path)
{
    FILE *fp;
    long size;
    char *buf;

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    size = ftell(fp);
    if (size <= 0) {
        fclose(fp);
        return NULL;
    }

    rewind(fp);

    buf = (char *)malloc((size_t)size + 1);
    if (buf == NULL) {
        fclose(fp);
        return NULL;
    }

    if (fread(buf, 1, (size_t)size, fp) != (size_t)size) {
        free(buf);
        fclose(fp);
        return NULL;
    }

    buf[size] = '\0';
    fclose(fp);

    return buf;
}

/*
 * @brief 读取字符串字段，缺失时保留默认值
 */
static void load_string_field(cJSON *root,
                              const char *name,
                              char *out,
                              int out_len)
{
    cJSON *item;

    if (root == NULL || name == NULL || out == NULL || out_len <= 0) {
        return;
    }

    item = cJSON_GetObjectItemCaseSensitive(root, name);
    if (cJSON_IsString(item) && item->valuestring != NULL) {
        snprintf(out, (size_t)out_len, "%s", item->valuestring);
    }
}

/*
 * @brief 读取整数字段，缺失时保留默认值
 */
static void load_int_field(cJSON *root, const char *name, int *out)
{
    cJSON *item;

    if (root == NULL || name == NULL || out == NULL) {
        return;
    }

    item = cJSON_GetObjectItemCaseSensitive(root, name);
    if (cJSON_IsNumber(item)) {
        *out = item->valueint;
    }
}

int app_config_load(const char *path)
{
    char *text;
    cJSON *root;
    cJSON *server;
    cJSON *storage;

    set_default_config();

    if (path == NULL) {
        LOG_WARN("app config path is null, use default config");
        return -1;
    }

    text = read_text_file(path);
    if (text == NULL) {
        LOG_WARN("read app config failed, path=%s, use default config", path);
        return -1;
    }

    root = cJSON_Parse(text);
    free(text);

    if (root == NULL) {
        LOG_WARN("parse app config failed, path=%s, use default config", path);
        return -1;
    }

    load_string_field(root, "device_id",
                      g_config.device_id, sizeof(g_config.device_id));
    load_string_field(root, "device_name",
                      g_config.device_name, sizeof(g_config.device_name));

    server = cJSON_GetObjectItemCaseSensitive(root, "server");
    if (cJSON_IsObject(server)) {
        load_string_field(server, "tcp_host",
                          g_config.tcp_host, sizeof(g_config.tcp_host));
        load_int_field(server, "tcp_port", &g_config.tcp_port);
    }

    storage = cJSON_GetObjectItemCaseSensitive(root, "storage");
    if (cJSON_IsObject(storage)) {
        load_string_field(storage, "db_path",
                          g_config.db_path, sizeof(g_config.db_path));
    }

    if (g_config.tcp_port <= 0 || g_config.tcp_port > 65535) {
        LOG_WARN("invalid tcp port in config, use default port");
        g_config.tcp_port = APP_CONFIG_DEFAULT_TCP_PORT;
    }

    cJSON_Delete(root);

    LOG_INFO("app config loaded, device_id=%s, tcp=%s:%d, db=%s",
             g_config.device_id,
             g_config.tcp_host,
             g_config.tcp_port,
             g_config.db_path);

    return 0;
}

const app_config_t *app_config_get(void)
{
    return &g_config;
}

const char *app_config_get_device_id(void)
{
    return g_config.device_id;
}
