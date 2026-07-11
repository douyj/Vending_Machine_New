#ifndef UI_SHOPPING_PAGE_H
#define UI_SHOPPING_PAGE_H

#include "lvgl.h"
#include "product/product_manager.h"

#define UI_SHOPPING_CART_MAX_ITEMS PRODUCT_MANAGER_PRODUCT_COUNT

typedef struct {
    product_info_t product;
    int quantity;
} ui_shopping_cart_item_t;

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *ui_shopping_page_create(void);
void ui_shopping_page_load(void);
int ui_shopping_cart_get_items(ui_shopping_cart_item_t *out_items, int max_count);
double ui_shopping_cart_get_total_price(void);
void ui_shopping_cart_clear(void);

#ifdef __cplusplus
}
#endif

#endif
