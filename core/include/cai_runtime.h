#ifndef CAI_RUNTIME_H
#define CAI_RUNTIME_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "cai_backend.h"

#define CAI_MAX_LABELS 16
#define CAI_LABEL_LEN 32

typedef struct {
    char labels[CAI_MAX_LABELS][CAI_LABEL_LEN];
    int label_count;
    float threshold;
} cai_model_meta_t;

typedef struct {
    const cai_backend_ops_t *backend;
    cai_model_meta_t meta;
    bool loaded;
    uint32_t infer_count;
    uint32_t last_infer_us;
} cai_runtime_t;

typedef struct {
    int top_index;
    float top_score;
    float scores[CAI_MAX_LABELS];
    int score_count;
} cai_infer_result_t;

int cai_runtime_init(cai_runtime_t *rt, const cai_backend_ops_t *backend);
int cai_runtime_load(cai_runtime_t *rt, const uint8_t *model, size_t model_len, const cai_model_meta_t *meta);
void cai_runtime_unload(cai_runtime_t *rt);
int cai_runtime_infer(cai_runtime_t *rt, const cai_tensor_t *input, cai_infer_result_t *result);
const char *cai_runtime_label(const cai_runtime_t *rt, int index);

const cai_backend_ops_t *cai_stub_backend(void);
const cai_backend_ops_t *cai_tflite_micro_backend(void);

#endif /* CAI_RUNTIME_H */
