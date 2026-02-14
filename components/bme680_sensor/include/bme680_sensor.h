#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "i2c_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    i2c_port_t i2c_port;
    gpio_num_t sda_io_num;
    gpio_num_t scl_io_num;
    uint32_t i2c_clk_speed_hz;
    uint8_t i2c_addr;
    uint16_t heater_temp_c;
    uint16_t heater_dur_ms;
    uint32_t auto_recalibration_interval_sec; /* 0 = default interval */
    uint32_t baseline_min_samples; /* BSEC IAQ-valid samples required, 0 = default */
    bool disable_auto_recalibration;
    bool disable_state_persistence;
} bme680_sensor_config_t;

typedef struct {
    float temperature_c;
    float pressure_pa;
    float humidity_rh;
    float gas_resistance_ohm;
    uint16_t iaq; /* 0..500, lower is better */
    uint8_t iaq_accuracy; /* 0..3 */
    bool stabilization_done;
    bool run_in_done;
    bool iaq_valid;
} bme680_sensor_data_t;

typedef enum {
    BME680_SENSOR_MODE_LP = 0,
    BME680_SENSOR_MODE_ULP,
} bme680_sensor_mode_t;

esp_err_t bme680_sensor_init(const bme680_sensor_config_t* config);
esp_err_t bme680_sensor_read(bme680_sensor_data_t* out_data);
esp_err_t bme680_sensor_set_mode(bme680_sensor_mode_t mode);
void bme680_sensor_deinit(void);
bool bme680_sensor_is_initialized(void);
bool bme680_sensor_is_calibrating(void);
void bme680_sensor_force_recalibration(void);
uint32_t bme680_sensor_get_next_call_delay_ms(void);

#ifdef __cplusplus
}
#endif
