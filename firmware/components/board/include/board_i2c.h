#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

// Creates the I2C master bus for the BME680 (SDA GPIO21, SCL GPIO22). The bus lives
// for the whole boot; the caller lends it to sensing, which adds its own device.
esp_err_t board_i2c_init(i2c_master_bus_handle_t* out);
