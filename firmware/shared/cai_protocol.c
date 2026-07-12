#include "cai_protocol.h"

#include <string.h>

uint8_t cai_crc8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

size_t cai_encode_frame(uint8_t cmd, const uint8_t *payload, uint8_t payload_len,
                        uint8_t *out, size_t out_cap)
{
    if (payload_len > CAI_MAX_PAYLOAD || out_cap < (size_t)(6 + payload_len)) {
        return 0;
    }

    uint8_t body[3 + CAI_MAX_PAYLOAD];
    body[0] = CAI_PROTO_VER;
    body[1] = cmd;
    body[2] = payload_len;
    if (payload_len > 0 && payload != NULL) {
        memcpy(&body[3], payload, payload_len);
    }

    out[0] = CAI_SOF0;
    out[1] = CAI_SOF1;
    memcpy(&out[2], body, 3 + payload_len);
    out[5 + payload_len] = cai_crc8(body, 3 + payload_len);
    return 6 + payload_len;
}

bool cai_decode_frame(const uint8_t *data, size_t len, cai_frame_t *out)
{
    if (len < 6 || data[0] != CAI_SOF0 || data[1] != CAI_SOF1) {
        return false;
    }
    if (data[2] != CAI_PROTO_VER) {
        return false;
    }

    uint8_t cmd = data[3];
    uint8_t plen = data[4];
    if (len < (size_t)(5 + plen + 1) || plen > CAI_MAX_PAYLOAD) {
        return false;
    }

    uint8_t crc = cai_crc8(&data[2], 3 + plen);
    if (crc != data[5 + plen]) {
        return false;
    }

    out->cmd = cmd;
    out->payload_len = plen;
    if (plen > 0) {
        memcpy(out->payload, &data[5], plen);
    }
    return true;
}

size_t cai_build_status_payload(const cai_status_t *st, uint8_t *out, size_t cap)
{
    if (!st || cap < 14) {
        return 0;
    }

    out[0] = st->loaded ? 1 : 0;
    out[1] = st->backend_kind;
    out[2] = st->label_count;
    out[3] = st->battery_pct;
    out[4] = st->flags;
    memcpy(&out[5], &st->infer_count, sizeof(uint32_t));
    out[9] = (uint8_t)(st->arena_kb & 0xff);
    out[10] = (uint8_t)((st->arena_kb >> 8) & 0xff);
    memcpy(&out[11], &st->threshold, sizeof(float));
    return 14;
}

size_t cai_build_result_payload(const cai_result_t *res, uint8_t *out, size_t cap)
{
    if (!res || cap < 8) {
        return 0;
    }

    out[0] = res->top_index;
    out[1] = res->score_count;
    out[2] = (uint8_t)(res->infer_us & 0xff);
    out[3] = (uint8_t)((res->infer_us >> 8) & 0xff);
    memcpy(&out[4], &res->top_score, sizeof(float));
    return 8;
}

bool cai_parse_threshold_payload(const uint8_t *payload, uint8_t len, float *out)
{
    if (!payload || len < 4 || !out) {
        return false;
    }
    memcpy(out, payload, sizeof(float));
    return true;
}
