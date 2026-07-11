#ifndef CHINESE_UI_FONTS_H
#define CHINESE_UI_FONTS_H

#if __has_include("lvgl.h")
#include "lvgl.h"
#elif __has_include("third_party/lvgl/lvgl.h")
#include "third_party/lvgl/lvgl.h"
#else
#error "Cannot find lvgl.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void ui_fonts_init(void);
const lv_font_t *ui_font_zh_12(void);
const lv_font_t *ui_font_zh_16(void);

#ifdef __cplusplus
}
#endif

#endif
