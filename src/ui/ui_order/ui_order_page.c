#include "ui/ui_order/ui_order_page.h"

#include <stdio.h>
#include <time.h>

#include "chinese/ui_fonts.h"
#include "storage/storage_manager.h"
#include "ui/ui_shopping/ui_shopping_page.h"

#define UI_ORDER_SCREEN_W 800
#define UI_ORDER_SCREEN_H 480
#define UI_ORDER_TOP_H    64
#define UI_ORDER_MARGIN   22
#define UI_ORDER_MAX_ROWS 20

static lv_obj_t *order_list;

static void style_plain_bg(lv_obj_t *obj, lv_color_t color)
{
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

static lv_obj_t *create_label(lv_obj_t *parent, const char *text,
                              const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    return label;
}

static const char *order_state_text(order_state_t state)
{
    switch (state) {
    case ORDER_STATE_DONE:
        return "完成";
    case ORDER_STATE_FAILED:
        return "失败";
    case ORDER_STATE_PENDING_UPLOAD:
        return "待上传";
    case ORDER_STATE_PAID:
        return "已支付";
    default:
        return "处理中";
    }
}

static void format_order_time(long create_time, char *buf, int len)
{
    struct tm *tm_info;
    time_t t = (time_t)create_time;

    if (buf == NULL || len <= 0) {
        return;
    }

    tm_info = localtime(&t);
    if (tm_info == NULL) {
        snprintf(buf, len, "--");
        return;
    }

    snprintf(buf, len, "%02d-%02d %02d:%02d",
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min);
}

static void back_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    ui_shopping_page_load();
}

static void create_top_bar(lv_obj_t *parent)
{
    lv_obj_t *top_bar = lv_obj_create(parent);
    lv_obj_set_size(top_bar, UI_ORDER_SCREEN_W, UI_ORDER_TOP_H);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(top_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(top_bar, lv_color_hex(0x2F6FD6), 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(top_bar, 0, 0);
    lv_obj_set_style_radius(top_bar, 0, 0);
    lv_obj_set_style_shadow_width(top_bar, 0, 0);
    lv_obj_set_style_pad_left(top_bar, UI_ORDER_MARGIN, 0);
    lv_obj_set_style_pad_right(top_bar, UI_ORDER_MARGIN, 0);

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

    lv_obj_t *title = create_label(top_bar, "订单记录",
                                   ui_font_zh_16(), lv_color_hex(0xFFFFFF));
    lv_obj_center(title);
}

static void create_order_row(lv_obj_t *parent, const order_info_t *order)
{
    char text[ORDER_PRODUCT_NAME_MAX_LEN + 32];
    char time_text[32];
    int line_count = 1;
    int row_height;

    for (const char *p = order->product_name; *p != '\0'; p++) {
        if (*p == '\n') {
            line_count++;
        }
    }

    row_height = 92 + (line_count - 1) * 24;

    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, UI_ORDER_SCREEN_W - UI_ORDER_MARGIN * 2 - 16,
                    row_height);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(row, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(row, 10, 0);
    lv_obj_set_style_border_color(row, lv_color_hex(0xE5EAF0), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_shadow_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);

    lv_obj_t *order_id = create_label(row, order->order_id,
                                      &lv_font_montserrat_14,
                                      lv_color_hex(0x6B7280));
    lv_obj_align(order_id, LV_ALIGN_TOP_LEFT, 14, 10);

    if (order->product_id == 0) {
        snprintf(text, sizeof(text), "%s", order->product_name);
    } else {
        snprintf(text, sizeof(text), "%s x%d", order->product_name,
                 order->product_count);
    }
    lv_obj_t *product = create_label(row, text, ui_font_zh_16(),
                                     lv_color_hex(0x111827));
    lv_obj_set_width(product, 320);
    lv_label_set_long_mode(product, LV_LABEL_LONG_WRAP);
    lv_obj_align(product, LV_ALIGN_TOP_LEFT, 14, 36);

    format_order_time(order->create_time, time_text, sizeof(time_text));
    lv_obj_t *time_label = create_label(row, time_text,
                                        &lv_font_montserrat_14,
                                        lv_color_hex(0x6B7280));
    lv_obj_align(time_label, LV_ALIGN_TOP_LEFT, 14, 64 + (line_count - 1) * 24);

    snprintf(text, sizeof(text), "%.2f元", order->total_price);
    lv_obj_t *price = create_label(row, text, ui_font_zh_16(),
                                   lv_color_hex(0x1677FF));
    lv_obj_set_width(price, 180);
    lv_obj_set_style_text_align(price, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(price, LV_ALIGN_RIGHT_MID, -120, 0);

    lv_obj_t *state = create_label(row, order_state_text(order->state),
                                   ui_font_zh_16(),
                                   order->state == ORDER_STATE_DONE ?
                                   lv_color_hex(0x16A34A) :
                                   lv_color_hex(0xDC2626));
    lv_obj_set_width(state, 90);
    lv_obj_set_style_text_align(state, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(state, LV_ALIGN_RIGHT_MID, -14, 0);
}

static void refresh_order_list(void)
{
    order_info_t orders[UI_ORDER_MAX_ROWS];
    int count;

    if (order_list == NULL) {
        return;
    }

    lv_obj_clean(order_list);
    count = storage_load_recent_orders(orders, UI_ORDER_MAX_ROWS);

    if (count <= 0) {
        lv_obj_t *empty = create_label(order_list, "暂无订单",
                                       ui_font_zh_16(),
                                       lv_color_hex(0x6B7280));
        lv_obj_center(empty);
        return;
    }

    for (int i = 0; i < count; i++) {
        create_order_row(order_list, &orders[i]);
    }
}

static void create_order_list(lv_obj_t *parent)
{
    order_list = lv_obj_create(parent);
    lv_obj_set_size(order_list,
                    UI_ORDER_SCREEN_W - UI_ORDER_MARGIN * 2,
                    UI_ORDER_SCREEN_H - UI_ORDER_TOP_H - UI_ORDER_MARGIN * 2);
    lv_obj_align(order_list, LV_ALIGN_TOP_MID, 0,
                 UI_ORDER_TOP_H + UI_ORDER_MARGIN);
    lv_obj_set_style_bg_color(order_list, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_bg_opa(order_list, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(order_list, 0, 0);
    lv_obj_set_style_pad_all(order_list, 8, 0);
    lv_obj_set_style_pad_row(order_list, 10, 0);
    lv_obj_set_flex_flow(order_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(order_list,
                          LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    refresh_order_list();
}

lv_obj_t *ui_order_page_create(void)
{
    ui_fonts_init();

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, UI_ORDER_SCREEN_W, UI_ORDER_SCREEN_H);
    style_plain_bg(screen, lv_color_hex(0xF8FAFC));
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    order_list = NULL;

    create_top_bar(screen);
    create_order_list(screen);

    return screen;
}

void ui_order_page_load(void)
{
    lv_scr_load_anim(ui_order_page_create(),
                     LV_SCR_LOAD_ANIM_NONE,
                     0,
                     0,
                     true);
}
