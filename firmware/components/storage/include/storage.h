#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

// Every call before a successful storage_init() returns ESP_ERR_INVALID_STATE.
// Functions are safe from any task; a failed write keeps the previous record.

typedef struct {
    uint8_t brightness_percent; // Range checks belong to the caller.
} storage_settings_t;

// Identifies the BSEC state record: a state is only restored into the same
// library version, configuration and sample-rate profile it was taken from.
typedef struct {
    uint32_t bsec_version; // major << 24 | minor << 16 | major_bugfix << 8 | minor_bugfix.
    uint32_t config_crc;   // CRC-32 of the configuration blob passed to BSEC.
    uint8_t profile;       // Sample-rate profile; each profile has its own record.
} storage_bsec_key_t;

// Opens the NVS partition. Never erases it: a full or newer-format partition is
// reported and the device runs without saved data.
esp_err_t storage_init(void);

// ESP_ERR_NOT_FOUND when nothing was saved yet, ESP_ERR_INVALID_VERSION for a
// record of another format.
esp_err_t storage_load_settings(storage_settings_t* out);
esp_err_t storage_save_settings(const storage_settings_t* settings);

// Copies the saved state into state[capacity]. ESP_ERR_NOT_FOUND when nothing was
// saved, ESP_ERR_INVALID_VERSION when the record belongs to another key and
// ESP_ERR_INVALID_CRC when it is damaged. *accuracy is the IAQ accuracy at saving.
esp_err_t storage_load_bsec_state(
    const storage_bsec_key_t* key,
    uint8_t* state,
    size_t capacity,
    size_t* length,
    uint8_t* accuracy);
esp_err_t storage_save_bsec_state(const storage_bsec_key_t* key, const uint8_t* state, size_t length, uint8_t accuracy);
