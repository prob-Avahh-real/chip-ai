#include "cai_tensor.h"

#include <string.h>

size_t cai_dtype_size(cai_dtype_t dtype)
{
    switch (dtype) {
    case CAI_DTYPE_FLOAT32:
        return 4;
    case CAI_DTYPE_INT8:
    case CAI_DTYPE_UINT8:
        return 1;
    default:
        return 0;
    }
}

size_t cai_tensor_byte_len(const cai_tensor_t *tensor)
{
    if (!tensor || tensor->ndim <= 0 || tensor->ndim > CAI_TENSOR_MAX_DIMS) {
        return 0;
    }

    size_t elems = 1;
    for (int i = 0; i < tensor->ndim; i++) {
        if (tensor->dims[i] <= 0) {
            return 0;
        }
        elems *= (size_t)tensor->dims[i];
    }
    return elems * cai_dtype_size(tensor->dtype);
}

int cai_tensor_init(cai_tensor_t *tensor, cai_dtype_t dtype, const int *dims, int ndim, void *data)
{
    if (!tensor || !dims || ndim <= 0 || ndim > CAI_TENSOR_MAX_DIMS || !data) {
        return -1;
    }

    tensor->dtype = dtype;
    tensor->ndim = ndim;
    memcpy(tensor->dims, dims, (size_t)ndim * sizeof(int));
    tensor->data = data;
    tensor->byte_len = cai_tensor_byte_len(tensor);
    return tensor->byte_len > 0 ? 0 : -1;
}

int cai_tensor_fill_f32(cai_tensor_t *tensor, float value)
{
    if (!tensor || tensor->dtype != CAI_DTYPE_FLOAT32 || !tensor->data) {
        return -1;
    }

    float *buf = (float *)tensor->data;
    size_t elems = tensor->byte_len / sizeof(float);
    for (size_t i = 0; i < elems; i++) {
        buf[i] = value;
    }
    return 0;
}
