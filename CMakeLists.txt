cmake_minimum_required(VERSION 3.10)

project(Vending_Machine C)

set(CMAKE_C_STANDARD 99)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

include_directories(
    inc
    .
    third_party/lvgl
    third_party/lv_drivers
    ${SDL2_INCLUDE_DIRS}
)

set(LV_CONF_PATH ${CMAKE_SOURCE_DIR}/lv_conf.h CACHE STRING "" FORCE)
add_subdirectory(third_party/lvgl EXCLUDE_FROM_ALL)

add_executable(Vending_Machine 
    src/main.c
    src/log/log.c
    src/product/product_manager.c
    src/member/member_manager.c
    src/order/order_manager.c
    src/order/order_state.c
    src/device/door_manager.c
    src/device/door_state.c
    src/storage/storage_manager.c
    src/app/app_state.c
    src/app/app_main.c
    src/ui/ui_login/ui_login_page.c
    src/ui/ui_login/ui_register_page.c
    src/ui/ui_shopping/ui_shopping_page.c
    src/ui/ui_shopping_car/ui_shopping_car_page.c
    src/chinese/font_chinese_12.c
    src/chinese/font_chinese_16.c
    src/chinese/ui_fonts.c
    third_party/lv_drivers/sdl/sdl.c
    third_party/lv_drivers/sdl/sdl_common.c
)

target_compile_options(Vending_Machine PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_libraries(Vending_Machine pthread sqlite3 lvgl::lvgl ${SDL2_LIBRARIES})
