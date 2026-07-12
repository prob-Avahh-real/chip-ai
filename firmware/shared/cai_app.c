#include "cai_app.h"

#include <string.h>

#define CAI_STATUS_INTERVAL_MS 2000
#define CAI_INFER_INTERVAL_MS 200

static void emit_notify(cai_notify_fn notify, uint8_t evt, const uint8_t *payload,
                        uint8_t len, void *ctx)
{
    if (notify != NULL) {
        notify(evt, payload, len, ctx);
    }
}

static void emit_error(cai_app_t *app, uint8_t code)
{
    if (app && app->ble.on_error) {
        app->ble.on_error(code, app->ble.ble_ctx);
    }
}

static cai_status_t cai_app_build_status(const cai_app_t *app)
{
    cai_status_t st = {0};
    if (!app) {
        return st;
    }

    st.loaded = app->runtime.loaded;
    st.infer_enabled = app->infer_enabled;
    st.backend_kind = (uint8_t)(app->runtime.backend ? app->runtime.backend->kind : CAI_BACKEND_STUB);
    st.label_count = (uint8_t)app->runtime.meta.label_count;
    st.battery_pct = app->board.read_battery_pct
        ? app->board.read_battery_pct(app->board.board_ctx) : 0;
    st.flags = (app->connected ? CAI_FLAG_CONNECTED : 0)
        | (app->infer_enabled ? CAI_FLAG_INFER_ENABLED : 0);
    st.infer_count = app->runtime.infer_count;
    st.arena_kb = app->runtime.backend && app->runtime.backend->arena_bytes
        ? (uint16_t)(app->runtime.backend->arena_bytes(app->runtime.backend->ctx) / 1024)
        : 0;
    st.threshold = app->threshold;
    return st;
}

void cai_app_init(cai_app_t *app, const cai_board_ops_t *board, const cai_ble_ops_t *ble)
{
    if (!app) {
        return;
    }
    memset(app, 0, sizeof(*app));
    if (board) {
        app->board = *board;
    }
    if (ble) {
        app->ble = *ble;
    }
    app->threshold = 0.5f;
    cai_runtime_init(&app->runtime, cai_stub_backend());
}

int cai_app_load_model(cai_app_t *app, const uint8_t *model, size_t model_len, const cai_model_meta_t *meta)
{
    if (!app) {
        return -1;
    }
    return cai_runtime_load(&app->runtime, model, model_len, meta);
}

void cai_app_set_connected(cai_app_t *app, bool connected)
{
    if (!app) {
        return;
    }
    app->connected = connected;
}

const cai_status_t *cai_app_status(const cai_app_t *app)
{
    static cai_status_t cached;
    if (!app) {
        return NULL;
    }
    cached = cai_app_build_status(app);
    return &cached;
}

static void cai_app_publish_status(cai_app_t *app)
{
    if (!app) {
        return;
    }

    cai_status_t st = cai_app_build_status(app);
    uint8_t pl[CAI_MAX_PAYLOAD];
    size_t plen = cai_build_status_payload(&st, pl, sizeof(pl));
    emit_notify(app->ble.notify_status, CAI_EVT_STATUS, pl, (uint8_t)plen, app->ble.ble_ctx);
}

static void cai_app_publish_result(cai_app_t *app, const cai_infer_result_t *result, uint16_t infer_us)
{
    if (!app || !result) {
        return;
    }

    cai_result_t res = {
        .top_index = (uint8_t)result->top_index,
        .score_count = (uint8_t)result->score_count,
        .infer_us = infer_us,
        .top_score = result->top_score,
    };

    uint8_t pl[CAI_MAX_PAYLOAD];
    size_t plen = cai_build_result_payload(&res, pl, sizeof(pl));
    emit_notify(app->ble.notify_result, CAI_EVT_RESULT, pl, (uint8_t)plen, app->ble.ble_ctx);
}

void cai_app_on_control_write(cai_app_t *app, const uint8_t *data, size_t len)
{
    if (!app || !data) {
        return;
    }

    cai_frame_t frame;
    if (!cai_decode_frame(data, len, &frame)) {
        emit_error(app, CAI_ERR_CRC_FAIL);
        return;
    }

    uint8_t pl[CAI_MAX_PAYLOAD];
    size_t plen;

    switch (frame.cmd) {
    case CAI_CMD_REQUEST_STATUS:
        cai_app_publish_status(app);
        break;

    case CAI_CMD_START_INFER:
        if (!app->runtime.loaded) {
            emit_error(app, CAI_ERR_MODEL_NOT_LOADED);
            break;
        }
        app->infer_enabled = true;
        cai_app_publish_status(app);
        break;

    case CAI_CMD_STOP_INFER:
        app->infer_enabled = false;
        cai_app_publish_status(app);
        break;

    case CAI_CMD_SET_THRESHOLD: {
        float value = 0.0f;
        if (!cai_parse_threshold_payload(frame.payload, frame.payload_len, &value)) {
            emit_error(app, CAI_ERR_UNSUPPORTED_CMD);
            break;
        }
        app->threshold = value;
        app->runtime.meta.threshold = value;
        cai_app_publish_status(app);
        break;
    }

    case CAI_CMD_PING:
        emit_notify(app->ble.notify_status, CAI_EVT_PONG, NULL, 0, app->ble.ble_ctx);
        break;

    default:
        emit_error(app, CAI_ERR_UNSUPPORTED_CMD);
        break;
    }

    (void)pl;
    (void)plen;
}

void cai_app_tick(cai_app_t *app)
{
    if (!app || !app->board.millis) {
        return;
    }

    uint32_t now = app->board.millis(app->board.board_ctx);

    if (!app->last_status_ms || now - app->last_status_ms >= CAI_STATUS_INTERVAL_MS) {
        cai_app_publish_status(app);
        app->last_status_ms = now;
    }

    if (!app->infer_enabled || !app->runtime.loaded) {
        return;
    }
    if (app->last_infer_ms && now - app->last_infer_ms < CAI_INFER_INTERVAL_MS) {
        return;
    }

    float input_buf[3] = {0};
    if (app->board.read_sensor_adc) {
        input_buf[0] = (float)app->board.read_sensor_adc(0, app->board.board_ctx) / 4095.0f;
        input_buf[1] = (float)app->board.read_sensor_adc(1, app->board.board_ctx) / 4095.0f;
        input_buf[2] = (float)app->board.read_sensor_adc(2, app->board.board_ctx) / 4095.0f;
    }

    int dims[1] = {3};
    cai_tensor_t input;
    if (cai_tensor_init(&input, CAI_DTYPE_FLOAT32, dims, 1, input_buf) != 0) {
        return;
    }

    cai_infer_result_t result;
    uint32_t t0 = now;
    if (cai_runtime_infer(&app->runtime, &input, &result) == 0) {
        uint32_t t1 = app->board.millis(app->board.board_ctx);
        uint16_t infer_us = (uint16_t)((t1 - t0) * 1000);
        if (infer_us == 0) {
            infer_us = 1;
        }
        cai_app_publish_result(&app, &result, infer_us);
        if (app->board.set_led) {
            app->board.set_led(result.top_score >= app->threshold, app->board.board_ctx);
        }
    }
    app->last_infer_ms = now;
}
