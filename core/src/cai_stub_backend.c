#include "cai_runtime.h"

#include <math.h>
#include <string.h>

typedef struct {
    float weights[3];
    float bias;
} cai_stub_ctx_t;

static cai_stub_ctx_t s_stub_ctx;

static int stub_load(void *ctx, const uint8_t *model, size_t model_len)
{
    (void)ctx;
    if (!model || model_len < 16) {
        return -1;
    }

    memcpy(&s_stub_ctx.weights[0], model, 12);
    memcpy(&s_stub_ctx.bias, model + 12, 4);
    return 0;
}

static int stub_unload(void *ctx)
{
    (void)ctx;
    memset(&s_stub_ctx, 0, sizeof(s_stub_ctx));
    return 0;
}

static int stub_infer(void *ctx, const cai_tensor_t *input, cai_tensor_t *output)
{
    (void)ctx;
    if (!input || !output || input->dtype != CAI_DTYPE_FLOAT32 || output->dtype != CAI_DTYPE_FLOAT32) {
        return -1;
    }

    const float *in = (const float *)input->data;
    float *out = (float *)output->data;
    size_t in_elems = input->byte_len / sizeof(float);
    size_t out_elems = output->byte_len / sizeof(float);
    if (in_elems < 3 || out_elems == 0) {
        return -1;
    }

    float logits[CAI_MAX_LABELS] = {0};
    int classes = (int)out_elems;
    if (classes > CAI_MAX_LABELS) {
        classes = CAI_MAX_LABELS;
    }

    for (int c = 0; c < classes; c++) {
        float sum = s_stub_ctx.bias + (float)c * 0.1f;
        for (size_t i = 0; i < 3 && i < in_elems; i++) {
            sum += in[i] * s_stub_ctx.weights[i];
        }
        logits[c] = sum;
    }

    float max_logit = logits[0];
    for (int c = 1; c < classes; c++) {
        if (logits[c] > max_logit) {
            max_logit = logits[c];
        }
    }

    float denom = 0.0f;
    for (int c = 0; c < classes; c++) {
        logits[c] = expf(logits[c] - max_logit);
        denom += logits[c];
    }
    if (denom <= 0.0f) {
        return -1;
    }

    for (int c = 0; c < classes; c++) {
        out[c] = logits[c] / denom;
    }
    return 0;
}

static size_t stub_arena_bytes(void *ctx)
{
    (void)ctx;
    return 2048;
}

static const cai_backend_ops_t s_stub_backend = {
    .name = "stub",
    .kind = CAI_BACKEND_STUB,
    .load = stub_load,
    .unload = stub_unload,
    .infer = stub_infer,
    .arena_bytes = stub_arena_bytes,
    .ctx = &s_stub_ctx,
};

const cai_backend_ops_t *cai_stub_backend(void)
{
    return &s_stub_backend;
}
