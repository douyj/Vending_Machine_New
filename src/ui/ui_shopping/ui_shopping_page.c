#include "ui/ui_shopping/ui_shopping_page.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "chinese/ui_fonts.h"
#include "member/member_manager.h"
#include "product/product_manager.h"
#include "ui/ui_order/ui_order_page.h"
#include "ui/ui_shopping_car/ui_shopping_car_page.h"

#define UI_SHOPPING_SCREEN_W        800
#define UI_SHOPPING_SCREEN_H        480
#define UI_SHOPPING_TOP_H           64
#define UI_SHOPPING_CATEGORY_H      56
#define UI_SHOPPING_MARGIN          22
#define UI_SHOPPING_CARD_W          170
#define UI_SHOPPING_CARD_H          154
#define UI_SHOPPING_QTY_BUTTON_SIZE 34

typedef enum {
    SHOPPING_CATEGORY_ALL = 0,
    SHOPPING_CATEGORY_DRINKS = PRODUCT_CATEGORY_DRINKS,
    SHOPPING_CATEGORY_SNACKS = PRODUCT_CATEGORY_SNACKS,
    SHOPPING_CATEGORY_FRUITS = PRODUCT_CATEGORY_FRUITS,
    SHOPPING_CATEGORY_OTHERS = PRODUCT_CATEGORY_OTHERS,
    SHOPPING_CATEGORY_COUNT
} shopping_category_t;

static lv_obj_t *category_labels[SHOPPING_CATEGORY_COUNT];
static lv_obj_t *quantity_labels[PRODUCT_MANAGER_PRODUCT_COUNT];
static product_info_t products[PRODUCT_MANAGER_PRODUCT_COUNT];
static int quantities[PRODUCT_MANAGER_PRODUCT_COUNT];
static int product_count;
static lv_obj_t *product_grid;
static lv_obj_t *total_price_label;
static lv_obj_t *balance_label;
static shopping_category_t selected_category = SHOPPING_CATEGORY_ALL;

/*
    @brief 设置对象的背景颜色为纯色，不透明度为100%，边框宽度为0，圆角为0，内边距为0
    @param obj 要设置的对象
    @param color 背景颜色
*/
static void style_plain_bg(lv_obj_t *obj, lv_color_t color)
{
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

/*
    @brief 创建一个标签
    @param parent 父对象
    @param text 标签文本
    @param font 字体
    @param color 字体颜色
    @return lv_obj_t* 标签对象
*/
static lv_obj_t *create_label(lv_obj_t *parent, const char *text,
                              const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    return label;
}

/*
    @brief 获取商品分类的图标
    @param category 商品分类
    @return const char* 商品分类的图标
*/
static const char *product_icon(product_category_t category)
{
    switch (category) {
    case PRODUCT_CATEGORY_DRINKS:
        return "饮";
    case PRODUCT_CATEGORY_SNACKS:
        return "零";
    case PRODUCT_CATEGORY_FRUITS:
        return "果";
    case PRODUCT_CATEGORY_OTHERS:
        return "其";
    default:
        return "?";
    }
}

/*
    @brief 获取商品分类的颜色
    @param category 商品分类
    @return uint32_t 商品分类的颜色
*/
static uint32_t product_color(product_category_t category)
{
    switch (category) {
    case PRODUCT_CATEGORY_DRINKS:
        return 0x3B82F6;
    case PRODUCT_CATEGORY_SNACKS:
        return 0xF97316;
    case PRODUCT_CATEGORY_FRUITS:
        return 0x22C55E;
    case PRODUCT_CATEGORY_OTHERS:
        return 0x64748B;
    default:
        return 0x94A3B8;
    }
}

/*
    @brief 设置商品卡片的启用状态
    @param obj 商品卡片对象
    @param enabled 是否启用
    @note 用于库存为0时候的样式
*/
static void set_product_enabled_style(lv_obj_t *obj, bool enabled)
{
    if (enabled) {
        lv_obj_clear_state(obj, LV_STATE_DISABLED);
        lv_obj_set_style_opa(obj, LV_OPA_COVER, 0);
    } else {
        lv_obj_add_state(obj, LV_STATE_DISABLED);
        lv_obj_set_style_opa(obj, LV_OPA_40, 0);
    }
}

/*
    @brief 更新商品分类的样式
    @note 用于更新商品分类的背景颜色、边框宽度、圆角、内边距等样式
*/
static void update_category_style(void)
{
    for (int i = 0; i < SHOPPING_CATEGORY_COUNT; i++) {
        lv_obj_t *btn;
        lv_obj_t *indicator;

        if (category_labels[i] == NULL) {
            continue;
        }

        btn = lv_obj_get_parent(category_labels[i]);
        indicator = lv_obj_get_child(btn, 1);
        if (i == selected_category) {
            lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(btn, 0, 0);
            lv_obj_set_style_text_color(category_labels[i], lv_color_hex(0x1677FF), 0);
            lv_obj_clear_flag(indicator, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(btn, 0, 0);
            lv_obj_set_style_text_color(category_labels[i], lv_color_hex(0x4B5563), 0);
            lv_obj_add_flag(indicator, LV_OBJ_FLAG_HIDDEN);
        }

        lv_obj_set_style_text_decor(category_labels[i], LV_TEXT_DECOR_NONE, 0);
    }
}

/*
    @brief 更新购物车商品数量的标签
    @param product_index 商品索引
    @note 用于更新商品数量的标签文本
*/
static void update_quantity_label(int product_index)
{
    char quantity_text[8];

    if (product_index < 0 ||
        product_index >= product_count ||
        quantity_labels[product_index] == NULL) {
        return;
    }

    snprintf(quantity_text, sizeof(quantity_text), "%d", quantities[product_index]);
    lv_label_set_text(quantity_labels[product_index], quantity_text);
}

/*
    @brief 计算购物车商品总价总价
    @return double 购物车商品总价总价
*/
static double calc_cart_total_price(void)
{
    double total_price = 0.0;

    for (int i = 0; i < product_count; i++) {
        total_price += products[i].product_price * quantities[i];
    }

    return total_price;
}

/*
    @brief 更新购物车商品总价的标签
    @note 用于更新商品总价的标签文本
*/
static void update_total_price_label(void)
{
    char total_text[32];

    if (total_price_label == NULL) {
        return;
    }

    snprintf(total_text, sizeof(total_text), "总价 %.2f元", calc_cart_total_price());
    lv_label_set_text(total_price_label, total_text);
}

static void update_balance_label(void)
{
    char balance_text[32];
    double balance = 0.0;

    if (balance_label == NULL) {
        return;
    }

    if (member_get_balance(&balance) == MEMBER_ERR_OK) {
        snprintf(balance_text, sizeof(balance_text), "余额 %.2f元", balance);
    } else {
        snprintf(balance_text, sizeof(balance_text), "余额 --");
    }

    lv_label_set_text(balance_label, balance_text);
}

/*
    @brief 处理商品数量按钮的点击事件
    @param event 事件对象
    @note "+""-"" 分别表示增加和减少商品数量    
*/
static void quantity_btn_event_cb(lv_event_t *event)
{
    int change;
    int product_index;
    product_info_t *product;

    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    change = (int)(intptr_t)lv_event_get_user_data(event);
    product_index = change > 0 ? change - 1 : -change - 1;

    if (product_index < 0 || product_index >= product_count) {
        return;
    }

    product = &products[product_index];
    if (product->product_stock <= 0) {
        return;
    }

    if (change > 0 && quantities[product_index] < product->product_stock) {
        quantities[product_index]++;
    } else if (change < 0 && quantities[product_index] > 0) {
        quantities[product_index]--;
    }

    update_quantity_label(product_index);
    update_total_price_label();
}

/*
    @brief 创建一个加号或减号按钮
    @param parent 父对象
    @param text 按钮文本
    @param bg_color 背景颜色
    @param text_color 文本颜色
    @param event_value 事件值
    @return lv_obj_t * 按钮对象
*/
static lv_obj_t *create_quantity_button(lv_obj_t *parent, const char *text,
                                        lv_color_t bg_color,
                                        lv_color_t text_color,
                                        int event_value)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, UI_SHOPPING_QTY_BUTTON_SIZE, UI_SHOPPING_QTY_BUTTON_SIZE);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(btn, bg_color, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 8, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, quantity_btn_event_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)event_value);

    lv_obj_t *label = create_label(btn, text, &lv_font_montserrat_20, text_color);
    lv_obj_center(label);

    return btn;
}

/*
    @brief 创建一个商品卡片
    @param parent 父对象
    @param product_index 商品索引
    @return lv_obj_t * 商品卡片对象
*/
static lv_obj_t *create_product_card(lv_obj_t *parent, int product_index)
{
    product_info_t *product = &products[product_index];
    char price_text[32];
    char stock_text[32];
    char quantity_text[8];
    bool can_buy = product->product_stock > 0;

    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, UI_SHOPPING_CARD_W, UI_SHOPPING_CARD_H);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xE5EAF0), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_shadow_width(card, 12, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_10, 0);
    lv_obj_set_style_shadow_ofs_y(card, 4, 0);
    lv_obj_set_style_pad_all(card, 12, 0);

    lv_obj_t *badge = lv_obj_create(card);
    lv_obj_set_size(badge, 44, 44);
    lv_obj_align(badge, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(badge, lv_color_hex(product_color(product->product_category)), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(badge, 10, 0);
    lv_obj_set_style_border_width(badge, 0, 0);

    lv_obj_t *icon = create_label(badge, product_icon(product->product_category),
                                  ui_font_zh_16(), lv_color_hex(0xFFFFFF));
    lv_obj_center(icon);

    lv_obj_t *name = create_label(card, product->product_name,
                                  ui_font_zh_16(), lv_color_hex(0x111827));
    lv_obj_align(name, LV_ALIGN_TOP_LEFT, 56, 1);

    snprintf(price_text, sizeof(price_text), "价格 %.2f元", product->product_price);
    lv_obj_t *price = create_label(card, price_text,
                                   ui_font_zh_12(), lv_color_hex(0x1677FF));
    lv_obj_align(price, LV_ALIGN_TOP_LEFT, 56, 28);

    if (!can_buy) {
        snprintf(stock_text, sizeof(stock_text), "已售罄");
    } else {
        snprintf(stock_text, sizeof(stock_text), "库存 %d", product->product_stock);
    }

    lv_obj_t *stock = create_label(card, stock_text,
                                   ui_font_zh_12(), lv_color_hex(0x6B7280));
    lv_obj_align(stock, LV_ALIGN_TOP_LEFT, 56, 52);

    lv_obj_t *minus_btn = create_quantity_button(card, "-",
                                                 lv_color_hex(0xEEF2F7),
                                                 lv_color_hex(0x374151),
                                                 -(product_index + 1));
    lv_obj_align(minus_btn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    set_product_enabled_style(minus_btn, can_buy);

    snprintf(quantity_text, sizeof(quantity_text), "%d",
             quantities[product_index]);
    quantity_labels[product_index] = create_label(card, quantity_text,
                                                  &lv_font_montserrat_18,
                                                  can_buy ? lv_color_hex(0x111827) :
                                                  lv_color_hex(0x9CA3AF));
    lv_obj_align(quantity_labels[product_index],
                 LV_ALIGN_BOTTOM_MID, 0, -7);

    lv_obj_t *plus_btn = create_quantity_button(card, "+",
                                                lv_color_hex(0x22A55A),
                                                lv_color_hex(0xFFFFFF),
                                                product_index + 1);
    lv_obj_align(plus_btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    set_product_enabled_style(plus_btn, can_buy);

    return card;
}

/*
    @brief 刷新商品卡片网格
    @note 用于根据当前选中的商品分类刷新商品卡片网格
*/
static void refresh_product_grid(void)
{
    if (product_grid == NULL) {
        return;
    }

    lv_obj_clean(product_grid);
    memset(quantity_labels, 0, sizeof(quantity_labels));

    for (int i = 0; i < product_count; i++) {
        if (selected_category != SHOPPING_CATEGORY_ALL &&
            products[i].product_category != (product_category_t)selected_category) {
            continue;
        }

        create_product_card(product_grid, i);
    }
}

/*
    @brief 加载所有商品
    @note 用于从商品管理器加载所有商品并更新商品卡片网格
*/
static void load_products(void)
{
    product_count = product_manager_get_all(products, PRODUCT_MANAGER_PRODUCT_COUNT);
    if (product_count < 0) {
        product_count = 0;
    }

    memset(quantity_labels, 0, sizeof(quantity_labels));
}

/*
    @brief 获取购物车中的商品项
    @param out_items 输出商品项数组
    @param max_count 最大商品项数量
    @return int 实际返回的商品项数量
*/
int ui_shopping_cart_get_items(ui_shopping_cart_item_t *out_items, int max_count)
{
    int item_count = 0;

    if (out_items == NULL || max_count <= 0) {
        return 0;
    }

    for (int i = 0; i < product_count && item_count < max_count; i++) {
        if (quantities[i] <= 0) {
            continue;
        }

        out_items[item_count].product = products[i];
        out_items[item_count].quantity = quantities[i];
        item_count++;
    }

    return item_count;
}

/*
    @brief 获取购物车中的总价
    @return double 总价金额
*/
double ui_shopping_cart_get_total_price(void)
{
    return calc_cart_total_price();
}

/*
    @brief 清空已选购物车数量
*/
void ui_shopping_cart_clear(void)
{
    memset(quantities, 0, sizeof(quantities));
    memset(quantity_labels, 0, sizeof(quantity_labels));
    product_count = product_manager_get_all(products, PRODUCT_MANAGER_PRODUCT_COUNT);
    if (product_count < 0) {
        product_count = 0;
    }
}

/*
    @brief 商品分类按钮点击事件回调函数
    @param event 事件对象
*/
static void category_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    selected_category = (shopping_category_t)(intptr_t)lv_event_get_user_data(event);
    update_category_style();
    refresh_product_grid();
}

/*
    @brief 购物车按钮点击事件回调函数
    @param event 事件对象
*/
static void cart_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    ui_shopping_car_page_load();
}

static void order_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    ui_order_page_load();
}

/*
    @brief 创建顶部导航栏
    @param parent 父对象
*/
static void create_top_bar(lv_obj_t *parent)
{
    lv_obj_t *top_bar = lv_obj_create(parent);
    lv_obj_set_size(top_bar, UI_SHOPPING_SCREEN_W, UI_SHOPPING_TOP_H);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x2F6FD6), 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_shadow_width(top_bar, 0, 0);
    lv_obj_set_style_pad_left(top_bar, UI_SHOPPING_MARGIN, 0);
    lv_obj_set_style_pad_right(top_bar, UI_SHOPPING_MARGIN, 0);

    lv_obj_t *title = create_label(top_bar, "会员购物",
                                   ui_font_zh_16(), lv_color_hex(0xFFFFFF));
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 0, 0);

    balance_label = create_label(top_bar, "余额 --",
                                 ui_font_zh_16(), lv_color_hex(0xFFFFFF));
    lv_obj_align_to(balance_label, title, LV_ALIGN_OUT_RIGHT_MID, 22, 0);

    lv_obj_t *cart_btn = lv_btn_create(top_bar);
    lv_obj_set_size(cart_btn, 112, 38);
    lv_obj_align(cart_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(cart_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(cart_btn, LV_OPA_20, 0);
    lv_obj_set_style_radius(cart_btn, 19, 0);
    lv_obj_set_style_border_color(cart_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_opa(cart_btn, LV_OPA_50, 0);
    lv_obj_set_style_border_width(cart_btn, 1, 0);
    lv_obj_set_style_shadow_width(cart_btn, 0, 0);
    lv_obj_set_style_pad_all(cart_btn, 0, 0);
    lv_obj_clear_flag(cart_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(cart_btn, cart_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cart_icon_bg = lv_obj_create(cart_btn);
    lv_obj_set_size(cart_icon_bg, 26, 26);
    lv_obj_align(cart_icon_bg, LV_ALIGN_LEFT_MID, 8, 0);
    lv_obj_clear_flag(cart_icon_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(cart_icon_bg, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(cart_icon_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(cart_icon_bg, 13, 0);
    lv_obj_set_style_border_width(cart_icon_bg, 0, 0);
    lv_obj_set_style_pad_all(cart_icon_bg, 0, 0);

    lv_obj_t *cart_icon = create_label(cart_icon_bg, "车",
                                       ui_font_zh_16(), lv_color_hex(0x2F6FD6));
    lv_obj_center(cart_icon);

    lv_obj_t *cart_label = create_label(cart_btn, "购物车",
                                        ui_font_zh_16(), lv_color_hex(0xFFFFFF));
    lv_obj_align(cart_label, LV_ALIGN_RIGHT_MID, -12, 0);

    lv_obj_t *order_btn = lv_btn_create(top_bar);
    lv_obj_set_size(order_btn, 74, 38);
    lv_obj_align_to(order_btn, cart_btn, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_obj_clear_flag(order_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(order_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(order_btn, LV_OPA_20, 0);
    lv_obj_set_style_radius(order_btn, 19, 0);
    lv_obj_set_style_border_color(order_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_opa(order_btn, LV_OPA_50, 0);
    lv_obj_set_style_border_width(order_btn, 1, 0);
    lv_obj_set_style_shadow_width(order_btn, 0, 0);
    lv_obj_set_style_pad_all(order_btn, 0, 0);
    lv_obj_add_event_cb(order_btn, order_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *order_label = create_label(order_btn, "订单",
                                         ui_font_zh_16(),
                                         lv_color_hex(0xFFFFFF));
    lv_obj_center(order_label);

    total_price_label = create_label(top_bar, "总价 0.00元",
                                     ui_font_zh_16(), lv_color_hex(0xFFFFFF));
    lv_obj_align_to(total_price_label, order_btn, LV_ALIGN_OUT_LEFT_MID, -18, 0);
}

/*
    @brief 创建商品分类栏
    @param parent 父对象
*/
static void create_category_bar(lv_obj_t *parent)
{
    static const char *categories[SHOPPING_CATEGORY_COUNT] = {
        "全部", "饮品", "零食", "水果", "其他"
    };

    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_set_size(bar, UI_SHOPPING_SCREEN_W, UI_SHOPPING_CATEGORY_H);
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, UI_SHOPPING_TOP_H);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(bar, lv_color_hex(0xD8E7FF), 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_pad_left(bar, UI_SHOPPING_MARGIN, 0);
    lv_obj_set_style_pad_right(bar, UI_SHOPPING_MARGIN, 0);
    lv_obj_set_style_pad_top(bar, 0, 0);
    lv_obj_set_style_pad_bottom(bar, 0, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar,
                          LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    for (int i = 0; i < SHOPPING_CATEGORY_COUNT; i++) {
        lv_obj_t *btn = lv_btn_create(bar);
        lv_obj_t *indicator;

        lv_obj_set_size(btn, 116, UI_SHOPPING_CATEGORY_H);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_add_event_cb(btn, category_btn_event_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);

        category_labels[i] = create_label(btn, categories[i],
                                          ui_font_zh_16(),
                                          lv_color_hex(0x4B5563));
        lv_obj_align(category_labels[i], LV_ALIGN_CENTER, 0, -2);

        indicator = lv_obj_create(btn);
        lv_obj_set_size(indicator, 34, 4);
        lv_obj_align(indicator, LV_ALIGN_BOTTOM_MID, 0, -6);
        lv_obj_clear_flag(indicator, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(indicator, lv_color_hex(0x1677FF), 0);
        lv_obj_set_style_bg_opa(indicator, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(indicator, 2, 0);
        lv_obj_set_style_border_width(indicator, 0, 0);
    }

    update_category_style();
}

/*
    @brief 创建商品区域
    @param parent 父对象
*/
static void create_product_area(lv_obj_t *parent)
{
    product_grid = lv_obj_create(parent);
    lv_obj_set_size(product_grid,
                    UI_SHOPPING_SCREEN_W - UI_SHOPPING_MARGIN * 2,
                    UI_SHOPPING_SCREEN_H - UI_SHOPPING_TOP_H -
                    UI_SHOPPING_CATEGORY_H - UI_SHOPPING_MARGIN);
    lv_obj_align(product_grid, LV_ALIGN_BOTTOM_MID, 0, -UI_SHOPPING_MARGIN);
    lv_obj_set_style_bg_color(product_grid, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_bg_opa(product_grid, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(product_grid, 0, 0);
    lv_obj_set_style_pad_all(product_grid, 8, 0);
    lv_obj_set_style_pad_row(product_grid, 14, 0);
    lv_obj_set_style_pad_column(product_grid, 14, 0);
    lv_obj_set_flex_flow(product_grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(product_grid,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);

    refresh_product_grid();
}

/*
    @brief 创建购物页面
    @return lv_obj_t * 购物页面对象
*/
lv_obj_t *ui_shopping_page_create(void)
{
    ui_fonts_init();

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, UI_SHOPPING_SCREEN_W, UI_SHOPPING_SCREEN_H);
    style_plain_bg(screen, lv_color_hex(0xF8FAFC));
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    selected_category = SHOPPING_CATEGORY_ALL;
    product_grid = NULL;
    total_price_label = NULL;
    balance_label = NULL;
    load_products();

    create_top_bar(screen);
    update_total_price_label();
    update_balance_label();
    create_category_bar(screen);
    create_product_area(screen);

    return screen;
}

/*
    @brief 加载购物页面 并显示在当前屏幕
*/
void ui_shopping_page_load(void)
{
    lv_scr_load_anim(ui_shopping_page_create(),
                     LV_SCR_LOAD_ANIM_NONE,
                     0,
                     0,
                     true);
}
