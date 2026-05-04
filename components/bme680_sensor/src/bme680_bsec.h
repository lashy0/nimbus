#pragma once

#include <stdint.h>

#include "bme68x.h"
#include "bme680_sensor.h"
#include "bsec_interface.h"
#include "esp_err.h"

esp_err_t bme680_bsec_apply_default_configuration(void);
esp_err_t bme680_bsec_update_subscription(bme680_sensor_mode_t mode);
esp_err_t bme680_bsec_apply_settings(const bsec_bme_settings_t* settings);
esp_err_t bme680_bsec_process_field(int64_t timestamp_ns, uint8_t op_mode, const struct bme68x_data* raw,
    uint32_t process_data_mask);
uint32_t bme680_bsec_measurement_wait_us(const bsec_bme_settings_t* settings);
uint32_t bme680_bsec_next_call_delay_ms(int64_t next_call_ns);
