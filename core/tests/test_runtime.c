#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "cai_runtime.h"

static uint8_t stub_model[16];

int main(void)
{
    float input_buf[3] = {0.2f, 0.5f, 0.1f};
    float weights[3] = {1.0f, 0.5f, -0.2f};
    float bias = 0.1f;

    memcpy(stub_model, weights, sizeof(weights));
    memcpy(stub_model + 12, &bias, sizeof(bias));

    cai_runtime_t rt;
    cai_model_meta_t meta = {
        .label_count = 3,
        .threshold = 0.4f,
    };
    strncpy(meta.labels[0], "idle", CAI_LABEL_LEN - 1);
    strncpy(meta.labels[1], "walk", CAI_LABEL_LEN - 1);
    strncpy(meta.labels[2], "run", CAI_LABEL_LEN - 1);

    assert(cai_runtime_init(&rt, cai_stub_backend()) == 0);
    assert(cai_runtime_load(&rt, stub_model, sizeof(stub_model), &meta) == 0);

    int dims[1] = {3};
    cai_tensor_t input;
    assert(cai_tensor_init(&input, CAI_DTYPE_FLOAT32, dims, 1, input_buf) == 0);

    cai_infer_result_t result;
    assert(cai_runtime_infer(&rt, &input, &result) == 0);
    assert(result.score_count == 3);
    assert(result.top_score > 0.0f && result.top_score <= 1.0f);

    float sum = 0.0f;
    for (int i = 0; i < result.score_count; i++) {
        sum += result.scores[i];
    }
    assert(sum > 0.99f && sum < 1.01f);

    printf("chip-ai core test ok: top=%s score=%.3f\n",
           cai_runtime_label(&rt, result.top_index), result.top_score);
    return 0;
}
