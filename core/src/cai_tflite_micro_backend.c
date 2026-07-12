#include "cai_tflite_micro_backend.h"
#include "cai_tensor.h"

#include <string.h>

#ifdef CAI_HAS_TFLITE_MICRO

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#ifndef CAI_TFLM_ARENA_SIZE
#define CAI_TFLM_ARENA_SIZE (16 * 1024)
#endif

static uint8_t s_tflm_arena[CAI_TFLM_ARENA_SIZE];
static const tflite::Model *s_model = nullptr;
static tflite::MicroInterpreter *s_interpreter = nullptr;
static bool s_tflm_ready = false;
static bool s_resolver_ready = false;

static tflite::MicroMutableOpResolver<8> *resolver(void)
{
    static tflite::MicroMutableOpResolver<8> s_resolver;
    if (!s_resolver_ready) {
        s_resolver.AddFullyConnected();
        s_resolver.AddSoftmax();
        s_resolver_ready = true;
    }
    return &s_resolver;
}

static int tflm_load(void *ctx, const uint8_t *model, size_t model_len)
{
    (void)ctx;
    if (!model || model_len < 8 || s_tflm_ready) {
        return s_tflm_ready ? 0 : -1;
    }

    s_model = tflite::GetModel(model);
    if (s_model->version() != TFLITE_SCHEMA_VERSION) {
        return -1;
    }

    static tflite::MicroInterpreter s_static_interpreter(
        s_model, *resolver(), s_tflm_arena, sizeof(s_tflm_arena));
    s_interpreter = &s_static_interpreter;

    if (s_interpreter->AllocateTensors() != kTfLiteOk) {
        s_interpreter = nullptr;
        s_model = nullptr;
        return -1;
    }

    s_tflm_ready = true;
    return 0;
}

static int tflm_unload(void *ctx)
{
    (void)ctx;
    s_tflm_ready = false;
    return 0;
}

static int tflm_infer(void *ctx, const cai_tensor_t *input, cai_tensor_t *output)
{
    (void)ctx;
    if (!s_tflm_ready || !s_interpreter || !input || !output || !input->data || !output->data) {
        return -1;
    }

    TfLiteTensor *in_tensor = s_interpreter->input(0);
    TfLiteTensor *out_tensor = s_interpreter->output(0);
    if (!in_tensor || !out_tensor) {
        return -1;
    }

    size_t in_bytes = input->byte_len;
    if ((size_t)in_tensor->bytes < in_bytes) {
        in_bytes = (size_t)in_tensor->bytes;
    }

    if (in_tensor->type == kTfLiteFloat32) {
        memcpy(in_tensor->data.f, input->data, in_bytes);
    } else {
        memcpy(in_tensor->data.uint8, input->data, in_bytes);
    }

    if (s_interpreter->Invoke() != kTfLiteOk) {
        return -1;
    }

    size_t out_bytes = output->byte_len;
    if ((size_t)out_tensor->bytes < out_bytes) {
        out_bytes = (size_t)out_tensor->bytes;
    }

    if (out_tensor->type == kTfLiteFloat32) {
        memcpy(output->data, out_tensor->data.f, out_bytes);
    } else {
        memcpy(output->data, out_tensor->data.uint8, out_bytes);
    }
    return 0;
}

static size_t tflm_arena_bytes(void *ctx)
{
    (void)ctx;
    return sizeof(s_tflm_arena);
}

static const cai_backend_ops_t s_tflm_backend = {
    .name = "tflite_micro",
    .kind = CAI_BACKEND_TFLITE_MICRO,
    .load = tflm_load,
    .unload = tflm_unload,
    .infer = tflm_infer,
    .arena_bytes = tflm_arena_bytes,
    .ctx = NULL,
};

bool cai_tflite_micro_is_available(void)
{
    return true;
}

const cai_backend_ops_t *cai_tflite_micro_backend(void)
{
    return &s_tflm_backend;
}

#else /* CAI_HAS_TFLITE_MICRO */

bool cai_tflite_micro_is_available(void)
{
    return false;
}

const cai_backend_ops_t *cai_tflite_micro_backend(void)
{
    return NULL;
}

#endif /* CAI_HAS_TFLITE_MICRO */
