#pragma once

#include <stdint.h>

#include "esp_err.h"

/**
 * @file
 * Board-owned GPIO4: safe LOW at startup, LEDC while active and RTC-held LOW
 * during display sleep and final deep sleep. Initialize from the startup task;
 * serialize subsequent calls through the UI. All callbacks ignore ctx.
 */

/**
 * Switches GPIO4 from the LOW output of board_init_safe_gpio() to LEDC at duty 0.
 * @return ESP_OK or an LEDC configuration error; partial setup is retained on error.
 */
esp_err_t board_backlight_init(void);

/**
 * Sets the initialized LEDC backlight to percent, 0..100%.
 * @return ESP_OK, ESP_ERR_INVALID_ARG for percent above 100, or an LEDC update error.
 */
esp_err_t board_backlight_set(void* ctx, uint8_t percent);

/**
 * Stops LEDC and holds GPIO4 LOW on the RTC mux through light sleep.
 * Call only after the UI has darkened the backlight and turned the panel off.
 * @return ESP_OK or an LEDC/GPIO error; completed steps are not rolled back.
 */
esp_err_t board_backlight_release(void* ctx);

/**
 * Reclaims GPIO4 after board_backlight_release(), keeping it LOW until LEDC drives duty 0.
 * @return ESP_OK or a GPIO/LEDC error; completed steps are not rolled back.
 */
esp_err_t board_backlight_reclaim(void* ctx);
