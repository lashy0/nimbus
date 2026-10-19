#pragma once

#include <stdint.h>

#include "esp_err.h"

// GPIO4 has one owner, board: LOW from board_init_safe_gpio(), LEDC here, and
// later the UI hand-over, RTC hold for sleep and safe-mode flashes.

// Switches GPIO4 from the LOW output of board_init_safe_gpio() to LEDC at duty 0.
esp_err_t board_backlight_init(void);

// percent 0..100. ctx is unused; the signature matches the UI backlight callback.
esp_err_t board_backlight_set(void* ctx, uint8_t percent);
