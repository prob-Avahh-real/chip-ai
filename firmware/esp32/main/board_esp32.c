#include "board_esp32.h"

#include "driver/gpio.h"
#include "esp_timer.h"

#define CAI_GPIO_LED GPIO_NUM_2
#define CAI_ADC_CH0 34
#define CAI_ADC_CH1 35
#define CAI_ADC_CH2 36

static uint16_t read_sensor_adc(uint8_t channel, void *ctx)
{
    (void)ctx;
    switch (channel) {
    case 0: return 1200;
    case 1: return 1800;
    case 2: return 900;
    default: return 0;
    }
}

static uint8_t read_battery_pct(void *ctx)
{
    (void)ctx;
    return 87;
}

static uint32_t millis(void *ctx)
{
    (void)ctx;
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static void set_led(bool on, void *ctx)
{
    (void)ctx;
    gpio_set_level(CAI_GPIO_LED, on ? 1 : 0);
}

static cai_board_ops_t s_ops;

void board_esp32_init(void)
{
    gpio_reset_pin(CAI_GPIO_LED);
    gpio_set_direction(CAI_GPIO_LED, GPIO_MODE_OUTPUT);
    gpio_set_level(CAI_GPIO_LED, 0);

    s_ops.read_sensor_adc = read_sensor_adc;
    s_ops.read_battery_pct = read_battery_pct;
    s_ops.millis = millis;
    s_ops.set_led = set_led;
    s_ops.board_ctx = NULL;
}

const cai_board_ops_t *board_esp32_ops(void)
{
    return &s_ops;
}
