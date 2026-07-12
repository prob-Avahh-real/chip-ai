#ifndef CAI_TENSOR_H
#define CAI_TENSOR_H

#include <stddef.h>
#include <stdint.h>

#define CAI_TENSOR_MAX_DIMS 4

typedef enum {
    CAI_DTYPE_FLOAT32 = 0,
    CAI_DTYPE_INT8,
    CAI_DTYPE_UINT8,
} cai_dtype_t;

typedef struct {
    cai_dtype_t dtype;
    int dims[CAI_TENSOR_MAX_DIMS];
    int ndim;
    void *data;
    size_t byte_len;
} cai_tensor_t;

size_t cai_dtype_size(cai_dtype_t dtype);
size_t cai_tensor_byte_len(const cai_tensor_t *tensor);
int cai_tensor_init(cai_tensor_t *tensor, cai_dtype_t dtype, const int *dims, int ndim, void *data);
int cai_tensor_fill_f32(cai_tensor_t *tensor, float value);

#endif /* CAI_TENSOR_H */
