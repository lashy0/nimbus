#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "soc/gpio_num.h"

/**
 * @file
 * Battery voltage from the ADC. Not thread-safe.
 */

/**
 * Sets up the ADC on the battery pin, and ADC_EN.
 *
 * @retval ESP_OK Readings are available.
 * @retval ESP_ERR_INVALID_ARG battery is not an ADC1 pin.
 * @retval ESP_ERR_NOT_SUPPORTED eFuse holds no ADC calibration.
 * @retval other ADC or GPIO errors.
 */
esp_err_t power_battery_init(gpio_num_t battery, gpio_num_t adc_enable);

/**
 * Reads the battery voltage. Blocks for about 5 ms.
 *
 * @param out_mv Non-NULL destination in millivolts at the battery; set only on ESP_OK.
 */
esp_err_t power_battery_read(uint32_t* out_mv);
