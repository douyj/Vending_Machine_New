#include "ui/ui_shopping_car/ui_shopping_car_page.h"

#include <stdio.h>

#include "chinese/ui_fonts.h"
#include "order/order_manager.h"
#include "ui/ui_shopping/ui_shopping_page.h"

#define UI_CART_SCREEN_W 800
#define UI_CART_SCREEN_H 480
#define UI_CART_TOP_H    64
#define UI_CART_MARGIN   22
#define UI_CART_BOTTOM_H 96

static lv_obj_t *cart_list;
static lv_obj_t *total_price_label;
static lv_obj_t *status_label;
static lv_obj_t *pay_dialog_overlay;
static lv_timer_t *pay_dialog_timer;

/*
    @brief 设置对象的背景颜色为纯色
    @param obj 对象
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
    @brief 创建标签
    @param parent 父对象
    @param text 标签文本
    @param font 字体
    @param color 字体颜色
    @return lv_obj_t * 标签对象
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

static const char *order_error_to_reason_text(int err)
{
    switch (err) {
    case ORDER_ERR_INVALID_PARAM:
        return "参数错误";
    case ORDER_ERR_PRODUCT_NOT_FOUND:
        return "商品不存在";
    case ORDER_ERR_OUT_OF_STOCK:
        return "库存不足";
    case ORDER_ERR_MEMBER_NOT_LOGIN:
        return "会员未登录";
    case ORDER_ERR_BALANCE_NOT_ENOUGH:
        return "余额不足";
    case ORDER_ERR_PAY_FAILED:
        return "扣款失败";
    case ORDER_ERR_STOCK_DEDUCT_FAILED:
        return "扣库存失败";
    default:
        return "未知错误";
    }
}

static void close_pay_dialog(bool delete_timer)
{
    lv_obj_t *overlay = pay_dialog_overlay;

    if (delete_timer && pay_dialog_timer != NULL) {
        lv_timer_del(pay_dialog_timer);
    }
    pay_dialog_timer = NULL;
    pay_dialog_overlay = NULL;

    if (overlay != NULL) {
        lv_obj_del(overlay);
    }
}

static void dialog_close_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    close_pay_dialog(true);
}

static void dialog_auto_close_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    close_pay_dialog(false);
}

static void show_pay_result_dialog(const char *title, const char *message,
                                   bool success)
{
    close_pay_dialog(true);

    lv_obj_t *overlay = lv_obj_create(lv_layer_top());
    pay_dialog_overlay = overlay;

    lv_obj_set_size(overlay, UI_CART_SCREEN_W, UI_CART_SCREEN_H);
    lv_obj_align(overlay, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_radius(overlay, 0, 0);
    lv_obj_set_style_pad_all(overlay, 0, 0);

    lv_obj_t *dialog = lv_obj_create(overlay);
    lv_obj_set_size(dialog, 360, 210);
    lv_obj_center(dialog);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(dialog, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(dialog, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(dialog, 12, 0);
    lv_obj_set_style_border_width(dialog, 0, 0);
    lv_obj_set_style_shadow_width(dialog, 18, 0);
    lv_obj_set_style_shadow_opa(dialog, LV_OPA_20, 0);
    lv_obj_set_style_pad_all(dialog, 0, 0);

    lv_obj_t *title_label = create_label(dialog, title, ui_font_zh_16(),
                                         success ? lv_color_hex(0x16A34A) :
                                         lv_color_hex(0xDC2626));
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 26);

    lv_obj_t *message_label = create_label(dialog, message, ui_font_zh_16(),
                                           lv_color_hex(0x374151));
    lv_obj_set_size(message_label, 300, 72);
    lv_label_set_long_mode(message_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(message_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(message_label, LV_ALIGN_TOP_MID, 0, 66);

    lv_obj_t *ok_btn = lv_obj_create(dialog);
    lv_obj_set_size(ok_btn, 112, 40);
    lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_MID, 0, -22);
    lv_obj_add_flag(ok_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(ok_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ok_btn, success ? lv_color_hex(0x22A55A) :
                              lv_color_hex(0xDC2626), 0);
    lv_obj_set_style_bg_opa(ok_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ok_btn, 10, 0);
    lv_obj_set_style_border_width(ok_btn, 0, 0);
    lv_obj_set_style_shadow_width(ok_btn, 0, 0);
    lv_obj_set_style_pad_all(ok_btn, 0, 0);
    lv_obj_add_event_cb(ok_btn, dialog_close_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *ok_label = create_label(ok_btn, "确定", ui_font_zh_16(),
                                      lv_color_hex(0xFFFFFF));
    lv_obj_center(ok_label);

    pay_dialog_timer = lv_timer_create(dialog_auto_close_timer_cb, 3000, NULL);
    lv_timer_set_repeat_count(pay_dialog_timer, 1);
}

static void update_total_price_label(void)
{
    char total_text[48];

    if (total_price_label == NULL) {
        return;
    }

    snprintf(total_text, sizeof(total_text), "总价 %.2f元",
             ui_shopping_cart_get_total_price());
    lv_label_set_text(total_price_label, total_text);
}

static void back_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    close_pay_dialog(true);
    ui_shopping_page_load();
}

static void create_top_bar(lv_obj_t *parent)
{
    lv_obj_t *top_bar = lv_obj_create(parent);
    lv_obj_set_size(top_bar, UI_CART_SCREEN_W, UI_CART_TOP_H);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x2F6FD6), 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_shadow_width(top_bar, 0, 0);
    lv_obj_set_style_pad_left(top_bar, UI_CART_MARGIN, 0);
    lv_obj_set_style_pad_right(top_bar, UI_CART_MARGIN, 0);

    lv_obj_t *back_btn = lv_btn_create(top_bar);
    lv_obj_set_size(back_btn, 82, 40);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(back_btn, 10, 0);
    lv_obj_set_style_shadow_width(back_btn, 0, 0);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_label = create_label(back_btn, "返回",
                                        ui_font_zh_16(), lv_color_hex(0x2F6FD6));
    lv_obj_center(back_label);

    lv_obj_t *title = create_label(top_bar, "购物车",
                                   ui_font_zh_16(), lv_color_hex(0xFFFFFF));
    lv_obj_center(title);
}

static void create_cart_row(lv_obj_t *parent, const ui_shopping_cart_item_t *item)
{
    char text[64];
    double subtotal = item->product.product_price * item->quantity;

    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, UI_CART_SCREEN_W - UI_CART_MARGIN * 2 - 16, 72);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(row, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 10, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0xE5EAF0), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_shadow_width(row, 0, 0);
    lv_obj_set_style_pad_left(row, 14, 0);
    lv_obj_set_style_pad_right(row, 14, 0);
    lv_obj_set_style_pad_top(row, 0, 0);
    lv_obj_set_style_pad_bottom(row, 0, 0);

    lv_obj_t *name = create_label(row, item->product.product_name,
                                  ui_font_zh_16(), lv_color_hex(0x111827));
    lv_obj_set_width(name, 250);
    lv_label_set_long_mode(name, LV_LABEL_LONG_CLIP);
    lv_obj_align(name, LV_ALIGN_LEFT_MID, 0, 0);

    snprintf(text, sizeof(text), "%.2f * %d", item->product.product_price, item->quantity);
    lv_obj_t *count = create_label(row, text,
                                   ui_font_zh_12(), lv_color_hex(0x6B7280));
    lv_obj_set_width(count, 190);
    lv_label_set_long_mode(count, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(count, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(count, LV_ALIGN_RIGHT_MID, -250, 0);

    snprintf(text, sizeof(text), " %.2f", subtotal);
    lv_obj_t *price = create_label(row, text,
                                   ui_font_zh_16(), lv_color_hex(0x1677FF));
    lv_obj_set_width(price, 230);
    lv_label_set_long_mode(price, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(price, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(price, LV_ALIGN_RIGHT_MID, 0, 0);
}

static void refresh_cart_list(void)
{
    ui_shopping_cart_item_t items[UI_SHOPPING_CART_MAX_ITEMS];
    int item_count;

    if (cart_list == NULL) {
        return;
    }

    lv_obj_clean(cart_list);
    item_count = ui_shopping_cart_get_items(items, UI_SHOPPING_CART_MAX_ITEMS);

    if (item_count <= 0) {
        lv_obj_t *empty = create_label(cart_list, "购物车空",
                                       ui_font_zh_16(), lv_color_hex(0x6B7280));
        lv_obj_center(empty);
        return;
    }

    for (int i = 0; i < item_count; i++) {
        create_cart_row(cart_list, &items[i]);
    }
}

/*
    @brief 支付按钮点击事件回调
    @param event 事件对象
*/
static void pay_btn_event_cb(lv_event_t *event)
{
    ui_shopping_cart_item_t items[UI_SHOPPING_CART_MAX_ITEMS];
    order_cart_item_t cart_items[UI_SHOPPING_CART_MAX_ITEMS];
    char message[96];
    double paid_total = 0.0;
    double balance_after = 0.0;
    int item_count;
    int ret = ORDER_ERR_OK;

    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    item_count = ui_shopping_cart_get_items(items, UI_SHOPPING_CART_MAX_ITEMS);
    if (item_count <= 0) {
        lv_label_set_text(status_label, "购物车空");
        show_pay_result_dialog("支付失败", "购物车空", false);
        return;
    }

    for (int i = 0; i < item_count; i++) {
        cart_items[i].product_id = items[i].product.product_id;
        cart_items[i].quantity = items[i].quantity;
    }

    ret = order_process_cart_buy(cart_items, item_count,
                                 &paid_total, &balance_after);
    if (ret == ORDER_ERR_OK) {
        ui_shopping_cart_clear();
        refresh_cart_list();
        update_total_price_label();
        lv_label_set_text(status_label, "");

        snprintf(message, sizeof(message), "共支付 %.2f元\n剩余余额 %.2f元",
                 paid_total, balance_after);
        show_pay_result_dialog("支付成功", message, true);
    } else {
        lv_label_set_text(status_label, "支付失败");
        snprintf(message, sizeof(message), "%s",
                 order_error_to_reason_text(ret));
        show_pay_result_dialog("支付失败", message, false);
    }
}

/*
    @brief 创建购物车列表
    @param parent 父对象
*/
static void create_cart_list(lv_obj_t *parent)
{
    cart_list = lv_obj_create(parent);
    lv_obj_set_size(cart_list,
                    UI_CART_SCREEN_W - UI_CART_MARGIN * 2,
                    UI_CART_SCREEN_H - UI_CART_TOP_H - UI_CART_BOTTOM_H -
                    UI_CART_MARGIN * 2);
    lv_obj_align(cart_list, LV_ALIGN_TOP_MID, 0, UI_CART_TOP_H + UI_CART_MARGIN);
    lv_obj_set_style_bg_color(cart_list, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_bg_opa(cart_list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(cart_list, 0, 0);
    lv_obj_set_style_pad_all(cart_list, 8, 0);
    lv_obj_set_style_pad_row(cart_list, 10, 0);
    lv_obj_set_flex_flow(cart_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cart_list,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    refresh_cart_list();
}

/*
    @brief 创建购物车底部栏
    @param parent 父对象
*/
static void create_bottom_bar(lv_obj_t *parent)
{
    lv_obj_t *bottom_bar = lv_obj_create(parent);
    lv_obj_set_size(bottom_bar, UI_CART_SCREEN_W, UI_CART_BOTTOM_H);
    lv_obj_align(bottom_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(bottom_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(bottom_bar, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(bottom_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(bottom_bar, lv_color_hex(0xD8E7FF), 0);
    lv_obj_set_style_border_width(bottom_bar, 1, 0);
    lv_obj_set_style_border_side(bottom_bar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_radius(bottom_bar, 0, 0);
    lv_obj_set_style_pad_left(bottom_bar, UI_CART_MARGIN, 0);
    lv_obj_set_style_pad_right(bottom_bar, UI_CART_MARGIN, 0);
    lv_obj_set_style_pad_top(bottom_bar, 0, 0);
    lv_obj_set_style_pad_bottom(bottom_bar, 0, 0);

    total_price_label = create_label(bottom_bar, "总价 0.00元",
                                     ui_font_zh_16(), lv_color_hex(0x111827));
    lv_obj_set_size(total_price_label, 260, 32);
    lv_label_set_long_mode(total_price_label, LV_LABEL_LONG_CLIP);
    lv_obj_align(total_price_label, LV_ALIGN_TOP_LEFT, 0, 18);
    update_total_price_label();

    status_label = create_label(bottom_bar, "",
                                ui_font_zh_12(), lv_color_hex(0x6B7280));
    lv_obj_set_size(status_label, 360, 26);
    lv_label_set_long_mode(status_label, LV_LABEL_LONG_CLIP);
    lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 0, 52);

    lv_obj_t *pay_btn = lv_obj_create(bottom_bar);
    lv_obj_set_size(pay_btn, 128, 46);
    lv_obj_align(pay_btn, LV_ALIGN_TOP_RIGHT, 0, 20);
    lv_obj_add_flag(pay_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(pay_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(pay_btn, lv_color_hex(0x22A55A), 0);
    lv_obj_set_style_bg_opa(pay_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(pay_btn, 10, 0);
    lv_obj_set_style_border_width(pay_btn, 0, 0);
    lv_obj_set_style_shadow_width(pay_btn, 0, 0);
    lv_obj_set_style_pad_all(pay_btn, 0, 0);
    lv_obj_add_event_cb(pay_btn, pay_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *pay_label = create_label(pay_btn, "支付",
                                       ui_font_zh_16(), lv_color_hex(0xFFFFFF));
    lv_obj_center(pay_label);
}

lv_obj_t *ui_shopping_car_page_create(void)
{
    ui_fonts_init();
    close_pay_dialog(true);

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, UI_CART_SCREEN_W, UI_CART_SCREEN_H);
    style_plain_bg(screen, lv_color_hex(0xF8FAFC));
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    cart_list = NULL;
    total_price_label = NULL;
    status_label = NULL;

    create_top_bar(screen);
    create_cart_list(screen);
    create_bottom_bar(screen);

    return screen;
}

void ui_shopping_car_page_load(void)
{
    lv_scr_load_anim(ui_shopping_car_page_create(),
                     LV_SCR_LOAD_ANIM_NONE,
                     0,
                     0,
                     true);
}
