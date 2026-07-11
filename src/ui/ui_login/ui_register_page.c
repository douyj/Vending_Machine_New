#include "ui/ui_login/ui_register_page.h"

#include "chinese/ui_fonts.h"
#include "ui/ui_login/ui_login_page.h"

#define UI_REGISTER_SCREEN_W       800
#define UI_REGISTER_SCREEN_H       480
#define UI_REGISTER_MARGIN         24
#define UI_REGISTER_LEFT_W         512
#define UI_REGISTER_LEFT_H         432
#define UI_REGISTER_PREVIEW_W      464
#define UI_REGISTER_PREVIEW_H      330
#define UI_REGISTER_RIGHT_W        216
#define UI_REGISTER_INPUT_W        204
#define UI_REGISTER_INPUT_H        58
#define UI_REGISTER_BUTTON_W       204
#define UI_REGISTER_BUTTON_H       76
#define UI_REGISTER_BACK_BUTTON_H  46

static lv_obj_t *create_label(lv_obj_t *parent, const char *text,
                              const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    return label;
}

static void style_plain_bg(lv_obj_t *obj, lv_color_t color)
{
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

static void create_preview_corner(lv_obj_t *parent, lv_align_t align,
                                  lv_coord_t x_ofs, lv_coord_t y_ofs)
{
    lv_obj_t *corner = lv_obj_create(parent);
    lv_obj_set_size(corner, 34, 34);
    lv_obj_align(corner, align, x_ofs, y_ofs);
    style_plain_bg(corner, lv_color_hex(0xFFFFFF));
    lv_obj_set_style_bg_opa(corner, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(corner, lv_color_hex(0x43B6FF), 0);
    lv_obj_set_style_border_width(corner, 3, 0);
    lv_obj_clear_flag(corner, LV_OBJ_FLAG_SCROLLABLE);

    if (align == LV_ALIGN_TOP_LEFT) {
        lv_obj_set_style_border_side(corner, LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_LEFT, 0);
    } else if (align == LV_ALIGN_TOP_RIGHT) {
        lv_obj_set_style_border_side(corner, LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_RIGHT, 0);
    } else if (align == LV_ALIGN_BOTTOM_LEFT) {
        lv_obj_set_style_border_side(corner, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_LEFT, 0);
    } else {
        lv_obj_set_style_border_side(corner, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_RIGHT, 0);
    }
}

static lv_obj_t *create_camera_area(lv_obj_t *screen)
{
    lv_obj_t *area = lv_obj_create(screen);
    lv_obj_set_size(area, UI_REGISTER_LEFT_W, UI_REGISTER_LEFT_H);
    lv_obj_align(area, LV_ALIGN_LEFT_MID, UI_REGISTER_MARGIN, 0);
    lv_obj_clear_flag(area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(area, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_bg_opa(area, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(area, 16, 0);
    lv_obj_set_style_border_color(area, lv_color_hex(0xE5EAF0), 0);
    lv_obj_set_style_border_width(area, 1, 0);
    lv_obj_set_style_shadow_width(area, 18, 0);
    lv_obj_set_style_shadow_opa(area, LV_OPA_10, 0);
    lv_obj_set_style_shadow_ofs_y(area, 6, 0);
    lv_obj_set_style_pad_all(area, 0, 0);

    lv_obj_t *title = create_label(area, "摄像头预览", ui_font_zh_16(),
                                   lv_color_hex(0x111827));
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 24, 18);

    lv_obj_t *hint = create_label(area, "请面向屏幕，保持面部在预览框内",
                                  ui_font_zh_12(), lv_color_hex(0x6B7280));
    lv_obj_align(hint, LV_ALIGN_TOP_LEFT, 24, 46);

    lv_obj_t *preview = lv_obj_create(area);
    lv_obj_set_size(preview, UI_REGISTER_PREVIEW_W, UI_REGISTER_PREVIEW_H);
    lv_obj_align(preview, LV_ALIGN_BOTTOM_MID, 0, -24);
    lv_obj_clear_flag(preview, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(preview, lv_color_hex(0x111827), 0);
    lv_obj_set_style_bg_opa(preview, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(preview, 12, 0);
    lv_obj_set_style_border_color(preview, lv_color_hex(0xD7DEE8), 0);
    lv_obj_set_style_border_width(preview, 2, 0);
    lv_obj_set_style_pad_all(preview, 0, 0);

    lv_obj_t *face_frame = lv_obj_create(preview);
    lv_obj_set_size(face_frame, 210, 250);
    lv_obj_center(face_frame);
    lv_obj_clear_flag(face_frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(face_frame, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(face_frame, lv_color_hex(0x43B6FF), 0);
    lv_obj_set_style_border_opa(face_frame, LV_OPA_30, 0);
    lv_obj_set_style_border_width(face_frame, 1, 0);
    lv_obj_set_style_radius(face_frame, 110, 0);

    create_preview_corner(preview, LV_ALIGN_TOP_LEFT, 18, 18);
    create_preview_corner(preview, LV_ALIGN_TOP_RIGHT, -18, 18);
    create_preview_corner(preview, LV_ALIGN_BOTTOM_LEFT, 18, -18);
    create_preview_corner(preview, LV_ALIGN_BOTTOM_RIGHT, -18, -18);

    lv_obj_t *status = create_label(preview, "等待摄像头画面",
                                    ui_font_zh_16(), lv_color_hex(0xE5E7EB));
    lv_obj_center(status);

    return area;
}

static lv_obj_t *create_face_button(lv_obj_t *parent)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, UI_REGISTER_BUTTON_W, UI_REGISTER_BUTTON_H);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x22A55A), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 12, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x22A55A), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_shadow_width(btn, 10, 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_10, 0);
    lv_obj_set_style_shadow_ofs_y(btn, 4, 0);

    lv_obj_t *label = create_label(btn, "人脸录入", ui_font_zh_16(),
                                   lv_color_hex(0xFFFFFF));
    lv_obj_center(label);

    return btn;
}

static lv_obj_t *create_back_button(lv_obj_t *parent)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, UI_REGISTER_BUTTON_W, UI_REGISTER_BACK_BUTTON_H);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xD7DEE8), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);

    lv_obj_t *label = create_label(btn, "返回", ui_font_zh_16(),
                                   lv_color_hex(0x374151));
    lv_obj_center(label);

    return btn;
}

static void back_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        ui_login_page_load();
    }
}

static lv_obj_t *create_action_area(lv_obj_t *screen)
{
    lv_obj_t *area = lv_obj_create(screen);
    lv_obj_set_size(area, UI_REGISTER_RIGHT_W, UI_REGISTER_LEFT_H);
    lv_obj_align(area, LV_ALIGN_RIGHT_MID, -UI_REGISTER_MARGIN, 0);
    style_plain_bg(area, lv_color_hex(0xFFFFFF));
    lv_obj_clear_flag(area, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = create_label(area, "会员注册", ui_font_zh_16(),
                                   lv_color_hex(0x111827));
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -116);

    lv_obj_t *name_label = create_label(area, "姓名", ui_font_zh_16(),
                                        lv_color_hex(0x374151));
    lv_obj_align(name_label, LV_ALIGN_CENTER, 0, -72);

    lv_obj_t *name_input = lv_textarea_create(area);
    lv_obj_set_size(name_input, UI_REGISTER_INPUT_W, UI_REGISTER_INPUT_H);
    lv_obj_align(name_input, LV_ALIGN_CENTER, 0, -28);
    lv_textarea_set_one_line(name_input, true);
    lv_textarea_set_placeholder_text(name_input, "请输入姓名");
    lv_textarea_set_max_length(name_input, 16);
    lv_obj_set_style_text_font(name_input, ui_font_zh_16(), 0);
    lv_obj_set_style_text_font(name_input, ui_font_zh_16(), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_color(name_input, lv_color_hex(0x111827), 0);
    lv_obj_set_style_text_color(name_input, lv_color_hex(0x9CA3AF),
                                LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_bg_color(name_input, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(name_input, lv_color_hex(0xD7DEE8), 0);
    lv_obj_set_style_border_width(name_input, 1, 0);
    lv_obj_set_style_radius(name_input, 10, 0);
    lv_obj_set_style_pad_left(name_input, 14, 0);
    lv_obj_set_style_pad_right(name_input, 14, 0);

    lv_obj_t *face_btn = create_face_button(area);
    lv_obj_align(face_btn, LV_ALIGN_CENTER, 0, 58);

    lv_obj_t *back_btn = create_back_button(area);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -18);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);

    return area;
}

lv_obj_t *ui_register_page_create(void)
{
    ui_fonts_init();

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, UI_REGISTER_SCREEN_W, UI_REGISTER_SCREEN_H);
    style_plain_bg(screen, lv_color_hex(0xFFFFFF));
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    create_camera_area(screen);
    create_action_area(screen);

    return screen;
}

void ui_register_page_load(void)
{
    lv_scr_load_anim(ui_register_page_create(),
                     LV_SCR_LOAD_ANIM_NONE,
                     0,
                     0,
                     true);
}
