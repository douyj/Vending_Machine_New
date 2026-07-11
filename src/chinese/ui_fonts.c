#include "chinese/ui_fonts.h"

#if __has_include("lvgl.h")
#include "lvgl.h"
#elif __has_include("third_party/lvgl/lvgl.h")
#include "third_party/lvgl/lvgl.h"
#else
#error "Cannot find lvgl.h"
#endif

LV_FONT_DECLARE(font_chinese_16);
LV_FONT_DECLARE(font_chinese_12);

void ui_fonts_init(void)
{
}

const lv_font_t *ui_font_zh_16(void)
{
    return &font_chinese_16;
}

const lv_font_t *ui_font_zh_12(void)
{
    return &font_chinese_12;
}
