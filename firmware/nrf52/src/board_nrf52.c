#include "board_nrf52.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define CAI_LED_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(CAI_LED_NODE, gpios);

static uint16_t read_sensor_adc(uint8_t channel, void *ctx)
{
    (void)ctx;
    return (uint16_t)(1000 + channel * 300);
}

static uint8_t read_battery_pct(void *ctx)
{
    (void)ctx;
    return 92;
}

static uint32_t millis(void *ctx)
{
    (void)ctx;
    return k_uptime_get_32();
}

static void set_led(bool on, void *ctx)
{
    (void)ctx;
    if (device_is_ready(led.port)) {
        gpio_pin_set_dt(&led, on ? 1 : 0);
    }
}

static cai_board_ops_t s_ops;

void board_nrf52_init(void)
{
    if (device_is_ready(led.port)) {
        gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    }

    s_ops.read_sensor_adc = read_sensor_adc;
    s_ops.read_battery_pct = read_battery_pct;
    s_ops.millis = millis;
    s_ops.set_led = set_led;
    s_ops.board_ctx = NULL;
}

const cai_board_ops_t *board_nrf52_ops(void)
{
    return &s_ops;
}
