#ifndef BOARD_NRF52_H
#define BOARD_NRF52_H

#include "cai_app.h"

void board_nrf52_init(void);
const cai_board_ops_t *board_nrf52_ops(void);

#endif /* BOARD_NRF52_H */
