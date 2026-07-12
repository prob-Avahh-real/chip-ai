#include "board_nrf52.h"
#include "ble_gatt.h"

#include "cai_app.h"

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(chip_ai, LOG_LEVEL_INF);

static cai_app_t s_app;

static uint8_t s_stub_model[16] = {
    0x00, 0x00, 0x80, 0x3f,
    0x00, 0x00, 0x00, 0x3f,
    0xcd, 0xcc, 0x4c, 0xbe,
    0xcd, 0xcc, 0xcc, 0x3d,
};

int main(void)
{
    board_nrf52_init();

    cai_ble_ops_t ble_ops = {
        .notify_status = ble_gatt_notify_status,
        .notify_result = ble_gatt_notify_result,
        .on_error = ble_gatt_notify_error,
        .ble_ctx = NULL,
    };
    cai_app_init(&s_app, board_nrf52_ops(), &ble_ops);

    cai_model_meta_t meta = {
        .label_count = 3,
        .threshold = 0.5f,
    };
    strncpy(meta.labels[0], "idle", CAI_LABEL_LEN - 1);
    strncpy(meta.labels[1], "walk", CAI_LABEL_LEN - 1);
    strncpy(meta.labels[2], "run", CAI_LABEL_LEN - 1);
    if (cai_app_load_model(&s_app, s_stub_model, sizeof(s_stub_model), &meta) != 0) {
        LOG_ERR("model load failed");
        return -1;
    }

    if (ble_gatt_init(&s_app) != 0) {
        LOG_ERR("ble init failed");
        return -1;
    }

    s_app.infer_enabled = true;
    LOG_INF("ChipAI nRF52 firmware ready");

    while (1) {
        cai_app_tick(&s_app);
        k_sleep(K_MSEC(10));
    }
    return 0;
}
