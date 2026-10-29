#pragma once

#include <stdint.h>

#include "esp_err.h"

#include "bme680.h"
#include "bsec_datatypes.h"

/**
 * @file
 * Glue between BSEC and the BME680 driver; only the BSEC data types and sizes
 * reach the rest of sensing. Uses the library's single built-in instance, so
 * one sensor at most. BSEC is not thread-safe: call these functions from one task only.
 */

/** BSEC outputs of one step. */
typedef struct {
    uint8_t count; /**< Outputs BSEC returned; 0 if none. */
    float temperature_c;
    float humidity_percent;
    float pressure_hpa;
    float iaq;
    uint8_t iaq_accuracy;
} sensing_bsec_outputs_t;

/**
 * Initializes BSEC with the embedded LP configuration and subscribes to the outputs.
 *
 * @retval ESP_OK BSEC is ready for sensing_bsec_step().
 * @retval ESP_FAIL BSEC rejected the configuration or the subscription.
 */
esp_err_t sensing_bsec_init(void);

/**
 * Loads a configuration blob, as bsec_iaq.txt holds it.
 *
 * @retval ESP_OK BSEC accepted the configuration.
 * @retval ESP_FAIL BSEC rejected it.
 */
esp_err_t sensing_bsec_set_config(const uint8_t* config, uint32_t size);

/**
 * Copies the current configuration blob.
 *
 * @param size Non-NULL; set to the bytes written to config.
 * @retval ESP_OK config holds the configuration.
 * @retval ESP_FAIL BSEC failed.
 */
esp_err_t sensing_bsec_get_config(uint8_t config[BSEC_MAX_PROPERTY_BLOB_SIZE], uint32_t* size);

/**
 * Restores a state saved with sensing_bsec_get_state(), calibration included.
 *
 * @retval ESP_OK BSEC accepted the state.
 * @retval ESP_FAIL BSEC rejected it.
 */
esp_err_t sensing_bsec_set_state(const uint8_t* state, uint32_t size);

/**
 * Copies the current state, calibration included, to keep across restarts.
 *
 * @param size Non-NULL; set to the bytes written to state.
 * @retval ESP_OK state holds the state.
 * @retval ESP_FAIL BSEC failed.
 */
esp_err_t sensing_bsec_get_state(uint8_t state[BSEC_MAX_STATE_BLOB_SIZE], uint32_t* size);

/**
 * Asks BSEC what to do now and, if it wants a measurement, runs it on the sensor
 * and processes the result. Blocks for the measurement.
 *
 * @param sensor Configured by this call before every measurement.
 * @param out Non-NULL; cleared, then filled with what BSEC produced.
 * @param next_call_ns Non-NULL; set to when BSEC wants the next step, in ns since
 *        boot, as soon as BSEC answers. Left unchanged if it does not.
 * @retval ESP_OK The step finished; out may still be empty.
 * @retval ESP_ERR_NOT_SUPPORTED BSEC asked for a mode other than forced.
 * @retval ESP_FAIL BSEC failed.
 * @retval other Errors of bme680_configure() and bme680_measure().
 */
esp_err_t sensing_bsec_step(bme680_handle_t sensor, sensing_bsec_outputs_t* out, int64_t* next_call_ns);
