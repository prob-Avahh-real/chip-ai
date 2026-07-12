#include "cai_runtime.h"

#include <string.h>

int cai_runtime_init(cai_runtime_t *rt, const cai_backend_ops_t *backend)
{
    if (!rt || !backend || !backend->load || !backend->infer) {
        return -1;
    }
    memset(rt, 0, sizeof(*rt));
    rt->backend = backend;
    rt->meta.threshold = 0.5f;
    return 0;
}

int cai_runtime_load(cai_runtime_t *rt, const uint8_t *model, size_t model_len, const cai_model_meta_t *meta)
{
    if (!rt || !rt->backend) {
        return -1;
    }

    if (rt->loaded) {
        cai_runtime_unload(rt);
    }

    if (rt->backend->load(rt->backend->ctx, model, model_len) != 0) {
        return -1;
    }

    if (meta) {
        rt->meta = *meta;
    } else {
        memset(&rt->meta, 0, sizeof(rt->meta));
        rt->meta.threshold = 0.5f;
    }
    rt->loaded = true;
    return 0;
}

void cai_runtime_unload(cai_runtime_t *rt)
{
    if (!rt || !rt->backend) {
        return;
    }
    if (rt->backend->unload) {
        rt->backend->unload(rt->backend->ctx);
    }
    rt->loaded = false;
}

int cai_runtime_infer(cai_runtime_t *rt, const cai_tensor_t *input, cai_infer_result_t *result)
{
    if (!rt || !rt->loaded || !input || !result || !rt->backend) {
        return -1;
    }

    float scores[CAI_MAX_LABELS] = {0};
    int dims[1] = {rt->meta.label_count > 0 ? rt->meta.label_count : 3};
    cai_tensor_t output;
    if (cai_tensor_init(&output, CAI_DTYPE_FLOAT32, dims, 1, scores) != 0) {
        return -1;
    }

    if (rt->backend->infer(rt->backend->ctx, input, &output) != 0) {
        return -1;
    }

    memset(result, 0, sizeof(*result));
    result->score_count = dims[0];
    int top = 0;
    float top_score = scores[0];
    for (int i = 0; i < dims[0] && i < CAI_MAX_LABELS; i++) {
        result->scores[i] = scores[i];
        if (scores[i] > top_score) {
            top_score = scores[i];
            top = i;
        }
    }

    result->top_index = top;
    result->top_score = top_score;
    rt->infer_count++;
    return 0;
}

const char *cai_runtime_label(const cai_runtime_t *rt, int index)
{
    if (!rt || index < 0 || index >= rt->meta.label_count) {
        return "unknown";
    }
    return rt->meta.labels[index];
}
