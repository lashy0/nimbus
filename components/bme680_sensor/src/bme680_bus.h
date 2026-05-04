#pragma once

#include <stdint.h>

#include "bme68x_defs.h"
#include "driver/gpio.h"

BME68X_INTF_RET_TYPE bme680_bus_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t length, void* intf_ptr);
BME68X_INTF_RET_TYPE bme680_bus_write(uint8_t reg_addr, const uint8_t* reg_data, uint32_t length, void* intf_ptr);
void bme680_bus_delay_us(uint32_t period, void* intf_ptr);
void bme680_bus_recovery(gpio_num_t sda_gpio, gpio_num_t scl_gpio);
