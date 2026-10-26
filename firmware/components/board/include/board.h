#pragma once

#include "esp_err.h"

/**
 * Initializes safe GPIO levels and releases holds left by deep sleep.
 * Call first from startup task context after reset or wake, before display, PWM
 * or ADC initialization. Drives backlight and ADC_EN LOW, keeps panel reset HIGH.
 * @return ESP_OK or the first GPIO setup error; earlier pin changes remain applied.
 */
esp_err_t board_init_safe_gpio(void);
