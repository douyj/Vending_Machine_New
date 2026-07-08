#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "product/product_manager.h"
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
    strcpy(g_products[0].product_name, "Coca Cola");
    g_products[0].product_price = 3.00;
    g_products[0].product_stock = 10;

    g_products[1].product_id = 2;
    strcpy(g_products[1].product_name, "Water");
    g_products[1].product_price = 2.00;
    g_products[1].product_stock = 15;

    g_products[2].product_id = 3;
    strcpy(g_products[2].product_name, "Bread");
    g_products[2].product_price = 5.00;
    g_products[2].product_stock = 8;

    g_products[3].product_id = 4;
    strcpy(g_products[3].product_name, "Tissue");
    g_products[3].product_price = 4.00;
    g_products[3].product_stock = 6;

    g_product_count = 4;

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
