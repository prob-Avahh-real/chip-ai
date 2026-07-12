#ifndef BOARD_ESP32_H
#define BOARD_ESP32_H

#include "cai_app.h"

void board_esp32_init(void);
const cai_board_ops_t *board_esp32_ops(void);

#endif /* BOARD_ESP32_H */
