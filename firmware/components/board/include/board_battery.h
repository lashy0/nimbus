#pragma once

#include <stdint.h>

#include "esp_err.h"

/**
 * Creates ADC1 channel 6 (GPIO34) with eFuse line-fitting calibration.
 * Call from the startup task after board_init_safe_gpio(). A failed init releases
 * the ADC and calibration; reads stay unavailable.
 * @return ESP_OK, ESP_ERR_NOT_SUPPORTED if eFuse calibration is unavailable,
 * or an ADC/calibration setup error.
 */
esp_err_t board_battery_init(void);

/**
 * Reads a new median-filtered battery-node voltage, correcting the 1:2 divider.
 *
 * Call from one task after board_battery_init(); synchronously samples the ADC
 * and waits for settling. On USB the value represents the charger rail, not the
 * cell. Attempts to drive ADC_EN LOW after sampling even when sampling fails.
 *
 * @param ctx Unused; matches the power read callback.
 * @param battery_mv Non-NULL output in millivolts; unchanged on error.
 * @return ESP_OK, ESP_ERR_INVALID_STATE before initialization, or an ADC/GPIO error.
 */
esp_err_t board_battery_read(void* ctx, uint32_t* battery_mv);
