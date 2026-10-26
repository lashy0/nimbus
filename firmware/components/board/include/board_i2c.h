#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

/**
 * Creates the BME680 I2C master bus on SDA GPIO21 and SCL GPIO22.
 * Call once from startup task context. The bus lives for the rest of the boot;
 * the caller lends it to sensing, which adds and owns its device.
 *
 * @param out Non-NULL destination; use the handle only after ESP_OK.
 * @return ESP_OK, ESP_ERR_INVALID_ARG for a NULL destination, or an I2C setup error.
 */
esp_err_t board_i2c_init(i2c_master_bus_handle_t* out);
