#pragma once

#include "esp_err.h"

// Call first after reset or wake, before display, PWM or ADC. Drives backlight and
// ADC_EN LOW, keeps panel reset HIGH and releases holds left by deep-sleep.
esp_err_t board_init_safe_gpio(void);
