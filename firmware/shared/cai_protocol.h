/**
 * ChipAI BLE 协议 v1 — 固件共享层
 * 与 mobile/flutter/lib/ble/protocol.dart 保持一致
 */
#ifndef CAI_PROTOCOL_H
#define CAI_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define CAI_PROTO_VER           1
#define CAI_SOF0                  0x43  /* 'C' */
#define CAI_SOF1                  0x41  /* 'A' */
#define CAI_MAX_PAYLOAD           32
#define CAI_BROADCAST_PREFIX      "ChipAI"

/* App → Chip */
#define CAI_CMD_REQUEST_STATUS    0x01
#define CAI_CMD_START_INFER       0x02
#define CAI_CMD_STOP_INFER        0x03
#define CAI_CMD_SET_THRESHOLD     0x04
#define CAI_CMD_PING              0x05

/* Chip → App */
#define CAI_EVT_STATUS            0x80
#define CAI_EVT_RESULT            0x81
#define CAI_EVT_PONG              0x85
#define CAI_EVT_ERROR             0x8F

#define CAI_ERR_CRC_FAIL          0x01
#define CAI_ERR_UNSUPPORTED_CMD   0x02
#define CAI_ERR_MODEL_NOT_LOADED  0x03

#define CAI_FLAG_CONNECTED        (1 << 0)
#define CAI_FLAG_INFER_ENABLED    (1 << 1)

#define CAI_UUID128_SERVICE       0xCA10A100, 0x4000, 0x8000, 0x0000, 0x000000CA1001
#define CAI_UUID128_CONTROL       0xCA10A103, 0x4000, 0x8000, 0x0000, 0x000000CA1001
#define CAI_UUID128_STATUS        0xCA10A101, 0x4000, 0x8000, 0x0000, 0x000000CA1001
#define CAI_UUID128_RESULT        0xCA10A102, 0x4000, 0x8000, 0x0000, 0x000000CA1001
#define CAI_UUID128_DEVICE_INFO   0xCA10A104, 0x4000, 0x8000, 0x0000, 0x000000CA1001

typedef struct {
    bool loaded;
    bool infer_enabled;
    uint8_t backend_kind;
    uint8_t label_count;
    uint8_t battery_pct;
    uint8_t flags;
    uint32_t infer_count;
    uint16_t arena_kb;
    float threshold;
} cai_status_t;

typedef struct {
    uint8_t top_index;
    uint8_t score_count;
    uint16_t infer_us;
    float top_score;
} cai_result_t;

typedef struct {
    uint8_t cmd;
    uint8_t payload[CAI_MAX_PAYLOAD];
    uint8_t payload_len;
} cai_frame_t;

typedef void (*cai_notify_fn)(uint8_t evt_cmd, const uint8_t *payload, uint8_t len, void *ctx);
typedef void (*cai_error_fn)(uint8_t code, void *ctx);

uint8_t cai_crc8(const uint8_t *data, size_t len);
size_t cai_encode_frame(uint8_t cmd, const uint8_t *payload, uint8_t payload_len,
                        uint8_t *out, size_t out_cap);
bool cai_decode_frame(const uint8_t *data, size_t len, cai_frame_t *out);

size_t cai_build_status_payload(const cai_status_t *st, uint8_t *out, size_t cap);
size_t cai_build_result_payload(const cai_result_t *res, uint8_t *out, size_t cap);
bool cai_parse_threshold_payload(const uint8_t *payload, uint8_t len, float *out);

#endif /* CAI_PROTOCOL_H */
