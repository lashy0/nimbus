#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_types.h"
#include "esp_err.h"

/**
 * @file
 * BME680 driver: finds the sensor on an I2C bus and runs single forced-mode
 * measurements. Not thread-safe: one task owns a handle.
 */

typedef struct bme680* bme680_handle_t;

/**
 * Settings for the next measurements.
 *
 * Oversampling uses the BME68x codes 0..5: skip the reading, x1, x2, x4, x8, x16.
 */
typedef struct {
    uint8_t temperature_os;
    uint8_t pressure_os;
    uint8_t humidity_os;
    bool gas_enabled;       /**< Heats the plate and measures gas resistance. */
    uint16_t heater_temp_c; /**< Target heater temperature, 200..400 °C; used with gas_enabled. */
    uint16_t heater_ms;     /**< Time at the target temperature, 1..4032 ms; used with gas_enabled. */
} bme680_config_t;

/** Compensated result of one measurement. */
typedef struct {
    float temperature_c;
    float humidity_percent; /**< 0..100 */
    float pressure_pa;
    float gas_ohm;  /**< Usable only with gas_valid. */
    bool gas_valid; /**< Gas was enabled and the sensor marked the gas reading valid. */
} bme680_sample_t;

/**
 * Finds the sensor at 0x77, then at 0x76, and initializes it.
 *
 * The bus is borrowed and must outlive the handle; the driver adds and owns its device.
 *
 * @param bus Initialized I2C master bus.
 * @param out Non-NULL destination; set only on ESP_OK.
 * @retval ESP_OK The sensor answered and was initialized.
 * @retval ESP_ERR_NOT_FOUND No device answered at either address.
 * @retval ESP_ERR_NO_MEM The handle could not be allocated.
 * @retval ESP_FAIL A device answered, but sensor initialization failed.
 */
esp_err_t bme680_open(i2c_master_bus_handle_t bus, bme680_handle_t* out);

/**
 * Writes the settings to the sensor; they apply to every measurement until the next call.
 *
 * A failure leaves the handle unconfigured until a later call succeeds.
 *
 * @retval ESP_OK The sensor accepted the settings.
 * @retval ESP_ERR_INVALID_ARG An oversampling value is out of range.
 * @retval ESP_FAIL A setting was rejected or a bus transfer failed.
 */
esp_err_t bme680_configure(bme680_handle_t dev, const bme680_config_t* config);

/**
 * Runs one forced-mode measurement with the current settings and waits for its result.
 *
 * Blocks the calling task for the conversion and the heater time.
 *
 * @param out Non-NULL destination; undefined on error.
 * @retval ESP_OK out holds a new measurement.
 * @retval ESP_ERR_INVALID_STATE bme680_configure() has not succeeded yet.
 * @retval ESP_ERR_TIMEOUT The sensor reported no new data after the wait.
 * @retval ESP_FAIL A bus transfer failed.
 */
esp_err_t bme680_measure(bme680_handle_t dev, bme680_sample_t* out);

/** Removes the device from the bus and frees the handle; NULL is ignored. */
void bme680_close(bme680_handle_t dev);
