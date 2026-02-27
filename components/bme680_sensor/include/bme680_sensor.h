#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "i2c_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration for BME680+BSEC sensor runtime.
 */
typedef struct {
    /**< I2C controller port. */
    i2c_port_t i2c_port;
    /**< I2C SDA GPIO number. */
    gpio_num_t sda_io_num;
    /**< I2C SCL GPIO number. */
    gpio_num_t scl_io_num;
    /**< I2C clock in Hz. */
    uint32_t i2c_clk_speed_hz;
    /**< BME680 I2C address (typically 0x76 or 0x77). */
    uint8_t i2c_addr;
    /**< Heater target temperature in Celsius. */
    uint16_t heater_temp_c;
    /**< Heater duration in milliseconds. */
    uint16_t heater_dur_ms;
    /**< Auto-recalibration period in seconds, 0 means default interval. */
    uint32_t auto_recalibration_interval_sec; /* 0 = default interval */
    /**< Minimum number of IAQ-valid samples for calibration completion, 0 means default. */
    uint32_t baseline_min_samples; /* BSEC IAQ-valid samples required, 0 = default */
    /**< Disable periodic auto-recalibration when true. */
    bool disable_auto_recalibration;
    /**< Disable BSEC state save/restore to NVS when true. */
    bool disable_state_persistence;
    /**< Reset BSEC baseline on power-on by clearing persisted state and skipping restore. */
    bool reset_baseline_on_power_on;
} bme680_sensor_config_t;

/**
 * @brief Processed sensor output snapshot.
 */
typedef struct {
    /**< Compensated temperature in Celsius. */
    float temperature_c;
    /**< Pressure in Pascals. */
    float pressure_pa;
    /**< Relative humidity in percent. */
    float humidity_rh;
    /**< Gas resistance in Ohms. */
    float gas_resistance_ohm;
    /**< IAQ value in range 0..500 (lower is better). */
    uint16_t iaq; /* 0..500, lower is better */
    /**< Static IAQ value in range 0..500 (lower is better). */
    uint16_t static_iaq; /* 0..500, lower is better */
    /**< IAQ accuracy in range 0..3. */
    uint8_t iaq_accuracy; /* 0..3 */
    /**< True when stabilization status is complete. */
    bool stabilization_done;
    /**< True when run-in status is complete. */
    bool run_in_done;
    /**< True when IAQ can be treated as valid for UI decisions. */
    bool iaq_valid;
} bme680_sensor_data_t;

/**
 * @brief Runtime sampling mode.
 */
typedef enum {
    /**< Low-power mode with higher sample rate than ULP. */
    BME680_SENSOR_MODE_LP = 0,
    /**< Ultra-low-power mode for reduced power consumption. */
    BME680_SENSOR_MODE_ULP,
} bme680_sensor_mode_t;

/**
 * @brief Initialize BME680 sensor, I2C bus, and BSEC processing.
 *
 * @param[in] config Sensor configuration.
 *
 * @return ESP_OK on success, otherwise an ESP error code.
 */
esp_err_t bme680_sensor_init(const bme680_sensor_config_t* config);

/**
 * @brief Read latest processed sensor data.
 *
 * @param[out] out_data Output structure for sensor data.
 *
 * @return ESP_OK on success, otherwise an ESP error code.
 */
esp_err_t bme680_sensor_read(bme680_sensor_data_t* out_data);

/**
 * @brief Change sensor sampling mode (LP/ULP).
 *
 * @param[in] mode Target mode.
 *
 * @return ESP_OK on success, otherwise an ESP error code.
 */
esp_err_t bme680_sensor_set_mode(bme680_sensor_mode_t mode);

/**
 * @brief Deinitialize sensor runtime and release resources.
 */
void bme680_sensor_deinit(void);

/**
 * @brief Check whether sensor runtime is initialized.
 *
 * @return true when initialized, false otherwise.
 */
bool bme680_sensor_is_initialized(void);

/**
 * @brief Check whether IAQ calibration is still in progress.
 *
 * @return true while calibrating, false when baseline is ready.
 */
bool bme680_sensor_is_calibrating(void);

/**
 * @brief Force recalibration cycle.
 */
void bme680_sensor_force_recalibration(void);

/**
 * @brief Get recommended delay until next sensor read.
 *
 * @return Delay in milliseconds.
 */
uint32_t bme680_sensor_get_next_call_delay_ms(void);

#ifdef __cplusplus
}
#endif
