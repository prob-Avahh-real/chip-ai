#ifndef CAI_BACKEND_H
#define CAI_BACKEND_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "cai_tensor.h"

typedef enum {
    CAI_BACKEND_STUB = 0,
    CAI_BACKEND_TFLITE_MICRO,
    CAI_BACKEND_ONNX,
    CAI_BACKEND_NCNN,
} cai_backend_kind_t;

typedef struct {
    const char *name;
    cai_backend_kind_t kind;
    int (*load)(void *ctx, const uint8_t *model, size_t model_len);
    int (*unload)(void *ctx);
    int (*infer)(void *ctx, const cai_tensor_t *input, cai_tensor_t *output);
    size_t (*arena_bytes)(void *ctx);
    void *ctx;
} cai_backend_ops_t;

#endif /* CAI_BACKEND_H */
