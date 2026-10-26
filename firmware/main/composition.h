#pragma once

#include "esp_err.h"

/**
 * Initializes board resources and connects the application components.
 * Call once from app_main(); blocks during hardware initialization and startup checks.
 * Low startup voltage enters sleep without returning. A sensor, storage or asset
 * failure alone does not fail composition. On error, partial resources remain alive.
 * @return ESP_OK or an error that requires composition_enter_fault_sleep().
 */
esp_err_t composition_init(void);

/**
 * Enters final sleep after composition_init() failed; never returns.
 * Call from task context. Darkens the backlight and resets the panel; an armed
 * button press boots again. May wait for panel reset and button release.
 */
__attribute__((noreturn)) void composition_enter_fault_sleep(void);
