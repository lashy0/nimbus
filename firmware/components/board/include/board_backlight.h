#pragma once

#include <stdint.h>

#include "esp_err.h"

// GPIO4 has one owner, board: LOW from board_init_safe_gpio(), LEDC here, held LOW
// while the display is off, and the RTC hold of the final deep sleep. ctx is unused
// in all functions; the signatures match the UI backlight callbacks.

// Switches GPIO4 from the LOW output of board_init_safe_gpio() to LEDC at duty 0.
esp_err_t board_backlight_init(void);

// percent 0..100.
esp_err_t board_backlight_set(void* ctx, uint8_t percent);

// The display is off: stops LEDC with its output LOW and holds GPIO4 LOW on the RTC
// mux, so the backlight stays dark through light sleep.
esp_err_t board_backlight_release(void* ctx);

// Undoes board_backlight_release(): GPIO4 stays LOW until LEDC drives it at duty 0.
esp_err_t board_backlight_reclaim(void* ctx);
