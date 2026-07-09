cmake_minimum_required(VERSION 3.10)

project(Vending_Machine C)

set(CMAKE_C_STANDARD 99)

include_directories(inc)

add_executable(Vending_Machine 
    src/main.c
    src/log/log.c
    src/product/product_manager.c
    src/member/member_manager.c
    src/order/order_manager.c
    src/order/order_state.c
    src/app/app_state.c
    src/app/app_main.c
)

target_link_libraries(Vending_Machine pthread)
