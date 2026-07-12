#include "board_esp32.h"
#include "ble_gatt.h"
#include "model_demo.h"

#include "cai_app.h"
#include "cai_tflite_micro_backend.h"

#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

static const char *TAG = "chip-ai";
static cai_app_t s_app;

static uint8_t s_stub_model[16] = {
    0x00, 0x00, 0x80, 0x3f,
    0x00, 0x00, 0x00, 0x3f,
    0xcd, 0xcc, 0x4c, 0xbe,
    0xcd, 0xcc, 0xcc, 0x3d,
};

static void app_task(void *arg)
{
    (void)arg;
    while (1) {
        cai_app_tick(&s_app);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void init_runtime_backend(cai_app_t *app)
{
#ifdef CAI_HAS_TFLITE_MICRO
    const cai_backend_ops_t *backend = cai_tflite_micro_backend();
    if (backend != NULL) {
        cai_runtime_init(&app->runtime, backend);
        ESP_LOGI(TAG, "backend: tflite_micro");
        return;
    }
#endif
    cai_runtime_init(&app->runtime, cai_stub_backend());
    ESP_LOGI(TAG, "backend: stub");
}

static int load_demo_model(cai_app_t *app, const cai_model_meta_t *meta)
{
#ifdef CAI_HAS_TFLITE_MICRO
    if (cai_tflite_micro_is_available()) {
        return cai_app_load_model(app, g_demo_tflite_model, g_demo_tflite_model_len, meta);
    }
#endif
    return cai_app_load_model(app, s_stub_model, sizeof(s_stub_model), meta);
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    board_esp32_init();

    cai_ble_ops_t ble_ops = {
        .notify_status = ble_gatt_notify_status,
        .notify_result = ble_gatt_notify_result,
        .on_error = ble_gatt_notify_error,
        .ble_ctx = NULL,
    };
    cai_app_init(&s_app, board_esp32_ops(), &ble_ops);
    init_runtime_backend(&s_app);

    cai_model_meta_t meta = {
        .label_count = 3,
        .threshold = 0.5f,
    };
    strncpy(meta.labels[0], "idle", CAI_LABEL_LEN - 1);
    strncpy(meta.labels[1], "walk", CAI_LABEL_LEN - 1);
    strncpy(meta.labels[2], "run", CAI_LABEL_LEN - 1);
    ESP_ERROR_CHECK(load_demo_model(&s_app, &meta) == 0 ? ESP_OK : ESP_FAIL);

    ESP_ERROR_CHECK(ble_gatt_init(&s_app) == 0 ? ESP_OK : ESP_FAIL);

    s_app.infer_enabled = true;
    xTaskCreate(app_task, "cai_app", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "ChipAI ESP32 firmware ready");
}
