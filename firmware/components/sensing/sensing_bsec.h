#pragma once

#include <stdint.h>

#include "esp_err.h"

#include "bme680.h"
#include "bsec_datatypes.h"

/**
 * @file
 * Runs BSEC on top of the BME680 driver. BSEC has a single built-in instance,
 * so one sensor at most. Not thread-safe: call from one task only.
 *
 * Start order: sensing_bsec_init(), optionally sensing_bsec_set_state(),
 * then sensing_bsec_subscribe().
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
 * Initializes BSEC with the embedded LP configuration.
 *
 * @retval ESP_OK BSEC is ready for sensing_bsec_set_state() or sensing_bsec_subscribe().
 * @retval ESP_FAIL BSEC failed to initialize or rejected the configuration.
 */
esp_err_t sensing_bsec_init(void);

/**
 * Restores a state saved with sensing_bsec_get_state(), calibration included.
 *
 * Call after sensing_bsec_init() and before sensing_bsec_subscribe().
 *
 * @retval ESP_OK BSEC accepted the state.
 * @retval ESP_FAIL BSEC rejected it.
 */
esp_err_t sensing_bsec_set_state(const uint8_t* state, uint32_t size);

/**
 * Subscribes to the outputs sensing publishes.
 *
 * @retval ESP_OK BSEC is ready for sensing_bsec_step().
 * @retval ESP_FAIL BSEC rejected the subscription.
 */
esp_err_t sensing_bsec_subscribe(void);

/**
 * Copies the current state, calibration included.
 *
 * @param size Non-NULL; set to the bytes written to state.
 * @retval ESP_OK The state is copied.
 * @retval ESP_FAIL BSEC failed.
 */
esp_err_t sensing_bsec_get_state(uint8_t state[BSEC_MAX_STATE_BLOB_SIZE], uint32_t* size);

/**
 * Asks BSEC what to do now and, if it wants a measurement, runs it on the sensor
 * and processes the result. Blocks for the measurement.
 *
 * @param sensor Reconfigured before every measurement.
 * @param out Non-NULL; cleared, then filled with what BSEC produced.
 * @param next_call_ns Non-NULL; when to call again, in ns since boot.
 *        Unchanged if BSEC did not answer.
 * @retval ESP_OK The step finished; out may still be empty.
 * @retval ESP_ERR_NOT_SUPPORTED BSEC asked for a mode other than forced.
 * @retval ESP_FAIL BSEC failed.
 * @retval other Errors of bme680_configure() and bme680_measure().
 */
esp_err_t sensing_bsec_step(bme680_handle_t sensor, sensing_bsec_outputs_t* out, int64_t* next_call_ns);
