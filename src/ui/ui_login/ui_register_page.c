#include "ui/ui_login/ui_register_page.h"

#include "chinese/ui_fonts.h"
#include "member/member_manager.h"
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
#define UI_REGISTER_INPUT_H        44
#define UI_REGISTER_BUTTON_W       204
#define UI_REGISTER_BUTTON_H       42
#define UI_REGISTER_BACK_BUTTON_H  40
#define UI_REGISTER_KEYBOARD_H     180

static lv_obj_t *username_input;
static lv_obj_t *password_input;
static lv_obj_t *name_input;
static lv_obj_t *status_label;
static lv_obj_t *keyboard;

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

static lv_obj_t *create_action_button(lv_obj_t *parent,
                                      const char *text,
                                      lv_color_t bg_color,
                                      lv_color_t text_color,
                                      lv_color_t border_color)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, UI_REGISTER_BUTTON_W, UI_REGISTER_BUTTON_H);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(btn, bg_color, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_border_color(btn, border_color, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_shadow_width(btn, 6, 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_10, 0);
    lv_obj_set_style_shadow_ofs_y(btn, 3, 0);

    lv_obj_t *label = create_label(btn, text, ui_font_zh_16(), text_color);
    lv_obj_center(label);

    return btn;
}

static lv_obj_t *create_register_input(lv_obj_t *parent,
                                       const char *placeholder,
                                       bool password_mode)
{
    lv_obj_t *input = lv_textarea_create(parent);
    lv_obj_set_size(input, UI_REGISTER_INPUT_W, UI_REGISTER_INPUT_H);
    lv_textarea_set_one_line(input, true);
    lv_textarea_set_placeholder_text(input, placeholder);
    lv_textarea_set_max_length(input, 24);
    lv_textarea_set_password_mode(input, password_mode);
    lv_obj_set_style_text_font(input, ui_font_zh_16(), 0);
    lv_obj_set_style_text_font(input, ui_font_zh_16(), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_color(input, lv_color_hex(0x111827), 0);
    lv_obj_set_style_text_color(input, lv_color_hex(0x9CA3AF),
                                LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_bg_color(input, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(input, lv_color_hex(0xD7DEE8), 0);
    lv_obj_set_style_border_width(input, 1, 0);
    lv_obj_set_style_radius(input, 10, 0);
    lv_obj_set_style_pad_left(input, 12, 0);
    lv_obj_set_style_pad_right(input, 12, 0);
    return input;
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
        if (keyboard != NULL) {
            lv_keyboard_set_textarea(keyboard, NULL);
            lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        }
        ui_login_page_load();
    }
}

static void hide_keyboard(void)
{
    if (keyboard == NULL) {
        return;
    }

    lv_keyboard_set_textarea(keyboard, NULL);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
}

static void input_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t *input = lv_event_get_target(event);

    if (keyboard == NULL) {
        return;
    }

    if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
        lv_keyboard_set_textarea(keyboard, input);
        lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
        lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(keyboard);
    }
}

static void keyboard_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        hide_keyboard();
    }
}

static void face_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    hide_keyboard();
    lv_label_set_text(status_label, "人脸录入功能待接入");
}

static void register_btn_event_cb(lv_event_t *event)
{
    const char *username;
    const char *password;
    const char *name;
    int ret;

    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    hide_keyboard();

    username = lv_textarea_get_text(username_input);
    password = lv_textarea_get_text(password_input);
    name = lv_textarea_get_text(name_input);

    ret = member_register(username, password, name);
    if (ret == MEMBER_ERR_OK) {
        lv_label_set_text(status_label, "");
        ui_login_page_load_with_message("注册成功");
        return;
    }

    if (ret == MEMBER_ERR_ALREADY_EXISTS) {
        lv_label_set_text(status_label, "账号已存在");
    } else if (ret == MEMBER_ERR_INVALID_PARAM) {
        lv_label_set_text(status_label, "请填写完整信息");
    } else {
        lv_label_set_text(status_label, "注册失败");
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
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -188);

    lv_obj_t *username_label = create_label(area, "账号", ui_font_zh_16(),
                                            lv_color_hex(0x374151));
    lv_obj_align(username_label, LV_ALIGN_CENTER, 0, -158);

    username_input = create_register_input(area, "请输入账号", false);
    lv_obj_align(username_input, LV_ALIGN_CENTER, 0, -126);
    lv_obj_add_event_cb(username_input, input_event_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(username_input, input_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *password_label = create_label(area, "密码", ui_font_zh_16(),
                                            lv_color_hex(0x374151));
    lv_obj_align(password_label, LV_ALIGN_CENTER, 0, -92);

    password_input = create_register_input(area, "请输入密码", true);
    lv_obj_align(password_input, LV_ALIGN_CENTER, 0, -60);
    lv_obj_add_event_cb(password_input, input_event_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(password_input, input_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *name_label = create_label(area, "姓名", ui_font_zh_16(),
                                        lv_color_hex(0x374151));
    lv_obj_align(name_label, LV_ALIGN_CENTER, 0, -26);

    name_input = create_register_input(area, "请输入姓名", false);
    lv_textarea_set_max_length(name_input, 16);
    lv_obj_align(name_input, LV_ALIGN_CENTER, 0, 6);
    lv_obj_add_event_cb(name_input, input_event_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(name_input, input_event_cb, LV_EVENT_CLICKED, NULL);

    status_label = create_label(area, "", ui_font_zh_12(),
                                lv_color_hex(0xDC2626));
    lv_obj_set_size(status_label, UI_REGISTER_BUTTON_W, 22);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 40);

    lv_obj_t *register_btn = create_action_button(area, "完成注册",
                                                  lv_color_hex(0x1677FF),
                                                  lv_color_hex(0xFFFFFF),
                                                  lv_color_hex(0x1677FF));
    lv_obj_align(register_btn, LV_ALIGN_CENTER, 0, 78);
    lv_obj_add_event_cb(register_btn, register_btn_event_cb,
                        LV_EVENT_CLICKED, NULL);

    lv_obj_t *face_btn = create_action_button(area, "人脸录入",
                                              lv_color_hex(0x7C3AED),
                                              lv_color_hex(0xFFFFFF),
                                              lv_color_hex(0x7C3AED));
    lv_obj_align(face_btn, LV_ALIGN_CENTER, 0, 126);
    lv_obj_add_event_cb(face_btn, face_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_btn = create_back_button(area);
    lv_obj_align(back_btn, LV_ALIGN_CENTER, 0, 176);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);

    return area;
}

static void create_keyboard(lv_obj_t *screen)
{
    keyboard = lv_keyboard_create(screen);
    lv_obj_set_size(keyboard, UI_REGISTER_SCREEN_W, UI_REGISTER_KEYBOARD_H);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_obj_set_style_bg_color(keyboard, lv_color_hex(0xF8FAFC), 0);
    lv_obj_set_style_bg_opa(keyboard, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(keyboard, lv_color_hex(0xD7DEE8), 0);
    lv_obj_set_style_border_width(keyboard, 1, 0);
    lv_obj_set_style_text_font(keyboard, &lv_font_montserrat_14, 0);
    lv_obj_add_event_cb(keyboard, keyboard_event_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(keyboard, keyboard_event_cb, LV_EVENT_CANCEL, NULL);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
}

lv_obj_t *ui_register_page_create(void)
{
    ui_fonts_init();

    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_size(screen, UI_REGISTER_SCREEN_W, UI_REGISTER_SCREEN_H);
    style_plain_bg(screen, lv_color_hex(0xFFFFFF));
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    username_input = NULL;
    password_input = NULL;
    name_input = NULL;
    status_label = NULL;
    keyboard = NULL;

    create_camera_area(screen);
    create_action_area(screen);
    create_keyboard(screen);

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
