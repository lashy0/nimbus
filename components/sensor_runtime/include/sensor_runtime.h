#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the sensor runtime worker task and UI timer.
 *
 * Spawns a FreeRTOS task that polls the BME680 sensor and battery, and
 * registers an LVGL timer that pushes the latest snapshot into the UI.
 * Also dispatches button events and idle-timeout logic.
 *
 * Must be called after the LVGL port is initialized and the UI is ready.
 *
 * @return ESP_OK on success, ESP_FAIL when the task or timer cannot be created.
 */
esp_err_t sensor_runtime_start(void);

#ifdef __cplusplus
}
#endif
