#ifndef CAI_TFLITE_MICRO_BACKEND_H
#define CAI_TFLITE_MICRO_BACKEND_H

#include "cai_backend.h"

const cai_backend_ops_t *cai_tflite_micro_backend(void);
bool cai_tflite_micro_is_available(void);

#endif /* CAI_TFLITE_MICRO_BACKEND_H */
