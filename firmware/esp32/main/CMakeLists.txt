idf_component_register(
    SRCS
        "main.c"
        "board_esp32.c"
        "ble_gatt.c"
        "model_demo.c"
        "../../shared/cai_app.c"
        "../../shared/cai_protocol.c"
        "../../../core/src/cai_runtime.c"
        "../../../core/src/cai_tensor.c"
        "../../../core/src/cai_stub_backend.c"
        "../../../core/src/cai_tflite_micro_backend.c"
    INCLUDE_DIRS
        "."
        "../../shared"
        "../../../core/include"
    REQUIRES
        bt
        nvs_flash
        esp_timer
)

if(TARGET idf::espressif__esp-tflite-micro)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE CAI_HAS_TFLITE_MICRO=1)
endif()
