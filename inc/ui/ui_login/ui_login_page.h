#ifndef UI_LOGIN_PAGE_H
#define UI_LOGIN_PAGE_H

#include "third_party/lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *ui_login_page_create(void);
void ui_login_page_load(void);
void ui_login_page_load_with_message(const char *message);

#ifdef __cplusplus
}
#endif

#endif
