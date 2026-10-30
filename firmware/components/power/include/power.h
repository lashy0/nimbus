#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "soc/gpio_num.h"

#include "power_types.h"

/**
 * @file
 * Battery and supply. Owns the battery ADC.
 */

typedef struct {
    gpio_num_t battery;    /**< ADC1 pin behind a 1:2 divider. */
    gpio_num_t adc_enable; /**< HIGH connects the divider. */
} power_config_t;

/**
 * Sets up the battery ADC, takes the first reading and starts the power task.
 *
 * Call once, from a task.
 *
 * @param config Non-NULL pins.
 * @param core Core for the power task, or tskNO_AFFINITY.
 * @retval ESP_OK The task is running.
 * @retval ESP_ERR_INVALID_ARG The chip has no such core, or battery is not an ADC1 pin.
 * @retval ESP_ERR_INVALID_STATE Power has already started.
 * @retval ESP_ERR_NOT_SUPPORTED eFuse holds no ADC calibration.
 * @retval ESP_ERR_NO_MEM The task could not be created.
 * @retval other ADC or GPIO errors.
 */
esp_err_t power_start(const power_config_t* config, BaseType_t core);

/**
 * Copies the latest status.
 *
 * Safe from any task, but not from an ISR. Without a running task the
 * source is POWER_SOURCE_UNKNOWN.
 *
 * @param out Non-NULL destination owned by the caller.
 */
void power_get_status(power_status_t* out);

/**
 * Whether the supply can start the display and the sensor, by the latest status.
 * After a brownout reset it needs a margin, and an unknown voltage is refused.
 *
 * Safe from any task. Without power_start() the voltage is unknown.
 */
bool power_startup_allowed(void);
