#pragma once

#include <stdint.h>

#include "esp_err.h"

// Creates ADC1 channel 6 (GPIO34) with eFuse line-fitting calibration. Without
// calibration it fails with ESP_ERR_NOT_SUPPORTED: raw counts are not a voltage.
esp_err_t board_battery_init(void);

// Median of a series of calibrated samples taken with ADC_EN HIGH, multiplied by
// the 1:2 divider of the board. ADC_EN is LOW again on every exit. The value is
// what GPIO34 sees: on USB it is the charger rail, not the cell. ctx is unused;
// the signature matches the power read callback.
esp_err_t board_battery_read(void* ctx, uint32_t* battery_mv);
