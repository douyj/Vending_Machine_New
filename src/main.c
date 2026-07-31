#include "lvgl.h"
#include "sdl/sdl.h"
#include <SDL2/SDL.h>

#include "app/app_config.h"
#include "product/product_manager.h"
#include "member/member_manager.h"
#include "order/order_manager.h"
#include "storage/storage_manager.h"
#include "device/door_manager.h"
#include "net/tcp_client.h"
#include "ui/ui_login/ui_login_page.h"
#include "ui/ui_shopping/ui_shopping_page.h"

#define APP_CONFIG_PATH "config/device_config.json"

int main(void)
{
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[SDL_HOR_RES * 80];
    const app_config_t *config;

    app_config_load(APP_CONFIG_PATH);
    config = app_config_get();
    lv_init();
    sdl_init();
    storage_manager_init(config->db_path);
    storage_create_tables();
    product_manager_init();     //对商品进行初始化
    member_manager_init();      //对会员进行初始化
    order_manager_init();       //对订单进行初始化
    door_manager_init();        //对柜门状态进行初始化
    tcp_client_start(config->tcp_host, config->tcp_port); //连接 Qt 后台


    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SDL_HOR_RES * 80);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = sdl_display_flush;
    disp_drv.hor_res = SDL_HOR_RES;
    disp_drv.ver_res = SDL_VER_RES;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t mouse_drv;
    lv_indev_drv_init(&mouse_drv);
    mouse_drv.type = LV_INDEV_TYPE_POINTER;
    mouse_drv.read_cb = sdl_mouse_read;
    lv_indev_drv_register(&mouse_drv);

    ui_login_page_load();

    while (!sdl_quit_qry) {
        ui_shopping_page_poll_refresh();
        lv_timer_handler();
        SDL_Delay(5);
        lv_tick_inc(5);
    }

    tcp_client_stop();
    storage_close();

    return 0;
}
