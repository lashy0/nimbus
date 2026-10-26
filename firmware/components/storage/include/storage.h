#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

/**
 * @file
 * Synchronous NVS access from task context; calls may block on flash operations.
 * Initialize before starting concurrent users. Load/save functions require valid,
 * non-NULL pointers and do not retain them. Without successful initialization,
 * NVS access returns ESP_ERR_INVALID_STATE. Concurrent task calls are supported.
 */

typedef struct {
    uint8_t brightness_percent; /**< Range checks belong to the caller. */
} storage_settings_t;

/** Identifies a BSEC state by its library version, configuration and sample-rate profile. */
typedef struct {
    uint32_t bsec_version; /**< major << 24 | minor << 16 | major_bugfix << 8 | minor_bugfix. */
    uint32_t config_crc;   /**< CRC-32 of the configuration blob passed to BSEC. */
    uint8_t profile;       /**< Sample-rate profile; each profile has its own record. */
} storage_bsec_key_t;

/**
 * Opens the NVS partition without erasing existing data.
 * @return ESP_OK or the NVS initialization error, including a full or newer-format partition.
 */
esp_err_t storage_init(void);

/**
 * Copies saved settings to out, which is unchanged on error.
 * @return ESP_OK, ESP_ERR_NOT_FOUND for an absent record, ESP_ERR_INVALID_VERSION
 * for another format, ESP_ERR_INVALID_SIZE for a malformed record, or an NVS error.
 */
esp_err_t storage_load_settings(storage_settings_t* out);
/**
 * Saves settings as one record and commits the write before returning.
 * Does not validate the brightness range.
 * @return ESP_OK or an NVS write/commit error.
 */
esp_err_t storage_save_settings(const storage_settings_t* settings);

/**
 * Copies a matching, CRC-checked BSEC state into the caller's buffer.
 * Output parameters are unchanged on error.
 *
 * @param key Expected version, configuration and profile.
 * @param state Destination with at least capacity writable bytes.
 * @param capacity Destination capacity in bytes.
 * @param length Receives the saved state's length in bytes.
 * @param accuracy Receives the IAQ accuracy recorded at saving, normally 0..3.
 * @return ESP_OK, ESP_ERR_NOT_FOUND for an absent record, ESP_ERR_INVALID_VERSION
 * for another format/key, ESP_ERR_INVALID_CRC for damaged state,
 * ESP_ERR_INVALID_SIZE for a malformed record or insufficient capacity,
 * ESP_ERR_NO_MEM for an allocation failure, or an NVS error.
 */
esp_err_t storage_load_bsec_state(
    const storage_bsec_key_t* key,
    uint8_t* state,
    size_t capacity,
    size_t* length,
    uint8_t* accuracy);
/**
 * Saves a BSEC state and its metadata as one record, then commits the write.
 * Input buffers are borrowed only for this call; accuracy is stored without validation.
 *
 * @param key Version, configuration and profile associated with the state.
 * @param state Non-NULL source with at least length readable bytes.
 * @param length State length in bytes, 1..512.
 * @param accuracy IAQ accuracy at saving, normally 0..3.
 * @return ESP_OK, ESP_ERR_INVALID_SIZE for an unsupported length,
 * ESP_ERR_NO_MEM for an allocation failure, or an NVS write/commit error.
 */
esp_err_t storage_save_bsec_state(const storage_bsec_key_t* key, const uint8_t* state, size_t length, uint8_t accuracy);
