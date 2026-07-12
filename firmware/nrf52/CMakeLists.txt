cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(chip_ai_nrf52)

target_sources(app PRIVATE
    src/main.c
    src/board_nrf52.c
    src/ble_gatt.c
    ../shared/cai_app.c
    ../shared/cai_protocol.c
    ../../core/src/cai_runtime.c
    ../../core/src/cai_tensor.c
    ../../core/src/cai_stub_backend.c
)

target_include_directories(app PRIVATE
    src
    ../shared
    ../../core/include
)
