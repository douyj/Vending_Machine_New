#ifndef __PRODUCT_MANAGER_H__
#define __PRODUCT_MANAGER_H__

#define PRODUCT_NAME_MAX_LEN 50
#define PRODUCT_MANAGER_PRODUCT_COUNT 12

typedef enum {
    PRODUCT_CATEGORY_DRINKS = 1,
    PRODUCT_CATEGORY_SNACKS,
    PRODUCT_CATEGORY_FRUITS,
    PRODUCT_CATEGORY_OTHERS,
} product_category_t;

typedef struct{
    int product_id;
    char product_name[PRODUCT_NAME_MAX_LEN];
    double product_price;
    int product_stock;
    product_category_t product_category;
}product_info_t;


void product_manager_init(void);
void product_printf_all(void);
int product_manager_get_all(product_info_t *out_products, int max_count);
int product_manager_get_by_id(int product_id, product_info_t *out_product);
int product_manager_set_stock(int product_id, int stock);
int product_manager_add_stock(int product_id, int count);
int product_manager_sub_stock(int product_id, int count);
int product_manager_has_stock(int product_id);



#endif
