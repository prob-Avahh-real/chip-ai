#ifndef CAI_APP_H
#define CAI_APP_H

#include <stdbool.h>
#include <stdint.h>

#include "cai_protocol.h"
#include "cai_runtime.h"

typedef struct cai_app cai_app_t;

typedef struct {
    uint16_t (*read_sensor_adc)(uint8_t channel, void *ctx);
    uint8_t (*read_battery_pct)(void *ctx);
    uint32_t (*millis)(void *ctx);
    void (*set_led)(bool on, void *ctx);
    void *board_ctx;
} cai_board_ops_t;

typedef struct {
    cai_notify_fn notify_status;
    cai_notify_fn notify_result;
    cai_error_fn on_error;
    void *ble_ctx;
} cai_ble_ops_t;

struct cai_app {
    cai_runtime_t runtime;
    cai_board_ops_t board;
    cai_ble_ops_t ble;
    bool connected;
    bool infer_enabled;
    float threshold;
    uint32_t last_status_ms;
    uint32_t last_infer_ms;
};

void cai_app_init(cai_app_t *app, const cai_board_ops_t *board, const cai_ble_ops_t *ble);
int cai_app_load_model(cai_app_t *app, const uint8_t *model, size_t model_len, const cai_model_meta_t *meta);
void cai_app_set_connected(cai_app_t *app, bool connected);
void cai_app_on_control_write(cai_app_t *app, const uint8_t *data, size_t len);
void cai_app_tick(cai_app_t *app);
const cai_status_t *cai_app_status(const cai_app_t *app);

#endif /* CAI_APP_H */
