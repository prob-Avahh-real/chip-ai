#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "cai_protocol.h"

static void test_roundtrip(uint8_t cmd, const uint8_t *payload, uint8_t payload_len)
{
    uint8_t frame[64];
    size_t n = cai_encode_frame(cmd, payload, payload_len, frame, sizeof(frame));
    assert(n > 0);

    cai_frame_t decoded;
    assert(cai_decode_frame(frame, n, &decoded));
    assert(decoded.cmd == cmd);
    assert(decoded.payload_len == payload_len);
    if (payload_len > 0) {
        assert(memcmp(decoded.payload, payload, payload_len) == 0);
    }
}

int main(void)
{
    test_roundtrip(CAI_CMD_START_INFER, NULL, 0);

    float threshold = 0.55f;
    test_roundtrip(CAI_CMD_SET_THRESHOLD, (const uint8_t *)&threshold, 4);

    cai_status_t st = {
        .loaded = true,
        .infer_enabled = true,
        .backend_kind = 1,
        .label_count = 3,
        .battery_pct = 90,
        .flags = CAI_FLAG_CONNECTED | CAI_FLAG_INFER_ENABLED,
        .infer_count = 7,
        .arena_kb = 16,
        .threshold = 0.5f,
    };
    uint8_t pl[32];
    size_t plen = cai_build_status_payload(&st, pl, sizeof(pl));
    assert(plen == 14);

    cai_result_t res = {
        .top_index = 2,
        .score_count = 3,
        .infer_us = 1200,
        .top_score = 0.73f,
    };
    plen = cai_build_result_payload(&res, pl, sizeof(pl));
    assert(plen == 8);

    printf("chip-ai protocol test ok\n");
    return 0;
}
