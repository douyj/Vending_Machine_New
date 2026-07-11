#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "product/product_manager.h"
#include "storage/storage_manager.h"
#include "log/log.h"

static int g_product_count = 0;

static product_info_t g_products[PRODUCT_MANAGER_PRODUCT_COUNT];
static pthread_mutex_t g_product_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
    @brief 初始化商品管理器
    @param none
    @return none
    @note 初始化商品管理器，将商品信息初始化为默认值
*/
void product_manager_init(void)
{
    pthread_mutex_lock(&g_product_mutex);

    memset(g_products, 0, sizeof(g_products));

    g_product_count = 0;

    g_products[0].product_id = 1;
    strcpy(g_products[0].product_name, "美式咖啡");
    g_products[0].product_price = 8.00;
    g_products[0].product_stock = 99;
    g_products[0].product_category = PRODUCT_CATEGORY_DRINKS;

    g_products[1].product_id = 2;
    strcpy(g_products[1].product_name, "拿铁咖啡");
    g_products[1].product_price = 10.00;
    g_products[1].product_stock = 99;
    g_products[1].product_category = PRODUCT_CATEGORY_DRINKS;

    g_products[2].product_id = 3;
    strcpy(g_products[2].product_name, "橙味汽水");
    g_products[2].product_price = 6.00;
    g_products[2].product_stock = 99;
    g_products[2].product_category = PRODUCT_CATEGORY_DRINKS;

    g_products[3].product_id = 4;
    strcpy(g_products[3].product_name, "薯片");
    g_products[3].product_price = 7.50;
    g_products[3].product_stock = 99;
    g_products[3].product_category = PRODUCT_CATEGORY_SNACKS;

    g_products[4].product_id = 5;
    strcpy(g_products[4].product_name, "巧克力");
    g_products[4].product_price = 5.50;
    g_products[4].product_stock = 99;
    g_products[4].product_category = PRODUCT_CATEGORY_SNACKS;

    g_products[5].product_id = 6;
    strcpy(g_products[5].product_name, "黄油饼干");
    g_products[5].product_price = 9.00;
    g_products[5].product_stock = 0;
    g_products[5].product_category = PRODUCT_CATEGORY_SNACKS;

    g_products[6].product_id = 7;
    strcpy(g_products[6].product_name, "苹果杯");
    g_products[6].product_price = 6.50;
    g_products[6].product_stock = 99;
    g_products[6].product_category = PRODUCT_CATEGORY_FRUITS;

    g_products[7].product_id = 8;
    strcpy(g_products[7].product_name, "香蕉");
    g_products[7].product_price = 4.00;
    g_products[7].product_stock = 99;
    g_products[7].product_category = PRODUCT_CATEGORY_FRUITS;

    g_products[8].product_id = 9;
    strcpy(g_products[8].product_name, "葡萄盒");
    g_products[8].product_price = 8.50;
    g_products[8].product_stock = 99;
    g_products[8].product_category = PRODUCT_CATEGORY_FRUITS;

    g_products[9].product_id = 10;
    strcpy(g_products[9].product_name, "纸巾包");
    g_products[9].product_price = 3.50;
    g_products[9].product_stock = 99;
    g_products[9].product_category = PRODUCT_CATEGORY_OTHERS;

    g_products[10].product_id = 11;
    strcpy(g_products[10].product_name, "湿巾");
    g_products[10].product_price = 4.50;
    g_products[10].product_stock = 99;
    g_products[10].product_category = PRODUCT_CATEGORY_OTHERS;

    g_products[11].product_id = 12;
    strcpy(g_products[11].product_name, "数据线");
    g_products[11].product_price = 15.00;
    g_products[11].product_stock = 99;
    g_products[11].product_category = PRODUCT_CATEGORY_OTHERS;

    g_product_count = PRODUCT_MANAGER_PRODUCT_COUNT;

    //从数据库中加载商品信息
    for(int i=0; i<g_product_count; i++)
    {
        product_info_t stored_product;
        if(storage_load_product(g_products[i].product_id, &stored_product) == STORAGE_ERR_OK)
        {
            g_products[i] = stored_product;
        }else{
            storage_insert_or_update_product(&g_products[i]);
        }
    }

    pthread_mutex_unlock(&g_product_mutex);

    LOG_INFO("product manager initialized, count=%d", g_product_count);
}

/*
    @brief 打印所有商品信息
    @param none
    @return none
    @note 打印商品管理器中的所有商品信息
*/
void product_printf_all(void)
{
    pthread_mutex_lock(&g_product_mutex);

    printf("\n========商品列表========\n");

    for(int i = 0; i < g_product_count; i++){
        product_info_t *p = &g_products[i];
        printf("%d. %-10s price=%.2f stock=%d\n",
            p->product_id,
            p->product_name,
            p->product_price,
            p->product_stock);
    }
    printf("======================\n\n");

    pthread_mutex_unlock(&g_product_mutex);
}

/*
    @brief 获取所有商品信息快照
    @param out_products 商品信息输出数组
    @param max_count 输出数组最大容量
    @return 实际写入的商品数量，失败返回 -1
*/
int product_manager_get_all(product_info_t *out_products, int max_count)
{
    int copy_count;

    if (out_products == NULL || max_count <= 0) {
        return -1;
    }

    pthread_mutex_lock(&g_product_mutex);

    copy_count = g_product_count < max_count ? g_product_count : max_count;
    memcpy(out_products, g_products, sizeof(product_info_t) * copy_count);

    pthread_mutex_unlock(&g_product_mutex);

    return copy_count;
}

/*
    @brief 根据商品ID获取商品信息
    @param product_id 商品ID
    @param out_product 商品信息指针
    @return 0 成功 -1 失败
    @note 根据商品ID从商品管理器中获取商品信息，将商品信息存储到out_product指针指向的内存中
*/
int product_manager_get_by_id(int product_id, product_info_t *out_product)
{
    if(product_id <= 0 || out_product == NULL) return -1;

    pthread_mutex_lock(&g_product_mutex);

    for(int i = 0; i < g_product_count; i++){
        if(g_products[i].product_id == product_id){
            *out_product = g_products[i];
            pthread_mutex_unlock(&g_product_mutex);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_product_mutex);
    return -1;
}

/*
    @brief 设置商品库存
    @param product_id 商品ID
    @param stock 最新库存
    @return 0 成功 -1 失败
*/
int product_manager_set_stock(int product_id, int stock)
{
    if(product_id <= 0 || stock < 0) {
        LOG_WARN("set stock invalid param, product_id=%d, stock=%d",
                 product_id, stock);
        return -1;
    }

    pthread_mutex_lock(&g_product_mutex);

    for(int i = 0; i < g_product_count; i++){
        if(g_products[i].product_id == product_id){
            g_products[i].product_stock = stock;
            pthread_mutex_unlock(&g_product_mutex);

            LOG_INFO("set stock success, product_id=%d, stock=%d",
                     product_id, stock);
            return 0;
        }
    }

    pthread_mutex_unlock(&g_product_mutex);
    LOG_WARN("set stock failed, product_id=%d not found", product_id);
    return -1;
}

/*
    @brief 增加商品库存
    @param product_id 商品ID
    @param count 库存数量
    @return 0 成功 -1 失败
    @note 增加商品库存，将商品库存增加count数量
*/
int product_manager_add_stock(int product_id, int count)
{
    if(product_id <= 0 || count <= 0) {
        LOG_WARN("add stock invalid param, product_id=%d, count=%d", product_id, count);
        return -1;
    }

    pthread_mutex_lock(&g_product_mutex);
    for(int i = 0; i < g_product_count; i++){
        if(g_products[i].product_id == product_id){
            g_products[i].product_stock += count;
            LOG_INFO("add stock success, product_id=%d, add=%d, stock=%d",
                     product_id, count, g_products[i].product_stock);
            pthread_mutex_unlock(&g_product_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&g_product_mutex);

    LOG_WARN("add stock failed, product_id=%d not found", product_id);
    return -1;
}

/*
    @brief 减少商品库存
    @param product_id 商品ID
    @param count 库存数量
    @return 0 成功 -1 失败
    @note 减少商品库存，将商品库存减少count数量 
*/
int product_manager_sub_stock(int product_id, int count)
{
    if(product_id <= 0 || count <= 0) {
        LOG_WARN("sub stock invalid param, product_id=%d, count=%d", product_id, count);
        return -1;
    }

    pthread_mutex_lock(&g_product_mutex);
    for(int i = 0; i < g_product_count; i++){
        if(g_products[i].product_id == product_id){
            if(g_products[i].product_stock < count){
                LOG_WARN("sub stock failed, product_id=%d, stock=%d, need=%d",
                         product_id, g_products[i].product_stock, count);
                pthread_mutex_unlock(&g_product_mutex);
                return -1;
            }

            g_products[i].product_stock -= count;
            storage_insert_or_update_product(&g_products[i]);   //更新商品库存
            LOG_INFO("sub stock success, product_id=%d, sub=%d, stock=%d",
                     product_id, count, g_products[i].product_stock);
            pthread_mutex_unlock(&g_product_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&g_product_mutex);

    LOG_WARN("sub stock failed, product_id=%d not found", product_id);
    return -1;
}

/*
    @brief 检查商品库存是否足够
    @param product_id 商品ID
    @return 1 有库存 0 无库存 -1 参数错误或商品不存在
    @note 检查商品库存是否足够
*/
int product_manager_has_stock(int product_id)
{
    if(product_id <= 0) {
        LOG_WARN("has stock invalid param, product_id=%d", product_id);
        return -1;
    }

    pthread_mutex_lock(&g_product_mutex);

    for(int i = 0; i < g_product_count; i++){
        if(g_products[i].product_id == product_id){
            int has_stock = (g_products[i].product_stock > 0);
            pthread_mutex_unlock(&g_product_mutex);
            return has_stock;
        }
    }

    pthread_mutex_unlock(&g_product_mutex);
    LOG_WARN("has stock failed, product_id=%d not found", product_id);
    return -1;
}
