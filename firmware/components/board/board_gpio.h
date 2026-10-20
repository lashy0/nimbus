#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

// For RTC-capable outputs such as GPIO4 and GPIO14. The RTC mux detaches the pin
// from LEDC or any other digital signal, and the hold keeps it LOW through light
// and deep sleep.
esp_err_t board_gpio_hold_low(gpio_num_t pin);

// Back to a digital LOW output: LOW is set on both muxes before the hold is
// released, so the pin never floats or goes HIGH on the way.
esp_err_t board_gpio_release_low(gpio_num_t pin);
