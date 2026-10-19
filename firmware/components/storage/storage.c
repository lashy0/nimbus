#include "storage.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_crc.h"
#include "nvs.h"
#include "nvs_flash.h"

#define SETTINGS_NAMESPACE "settings"
#define SETTINGS_KEY "settings"
#define SETTINGS_FORMAT 1

#define BSEC_NAMESPACE "bsec"
#define BSEC_FORMAT 1
#define BSEC_STATE_MAX_BYTES 512 // BSEC 2.6 states take 238 bytes; this only bounds a damaged record.

static const char* TAG = "storage";

typedef struct {
    uint8_t format;
    uint8_t brightness_percent;
} settings_record_t;

typedef struct {
    uint8_t format;
    uint8_t profile;
    uint8_t accuracy;
    uint8_t reserved;
    uint32_t bsec_version;
    uint32_t config_crc;
    uint32_t length;
    uint32_t state_crc;
} bsec_header_t;

static bool s_ready;

esp_err_t storage_init(void)
{
    const esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGE(TAG, "NVS needs erasing (%s); running without saved data", esp_err_to_name(err));

        return err;
    }

    ESP_RETURN_ON_ERROR(err, TAG, "NVS init");
    s_ready = true;

    return ESP_OK;
}

static esp_err_t open_namespace(const char* name, nvs_open_mode_t mode, nvs_handle_t* handle)
{
    ESP_RETURN_ON_FALSE(s_ready, ESP_ERR_INVALID_STATE, TAG, "not initialized");

    const esp_err_t err = nvs_open(name, mode, handle);

    // A namespace appears with its first write.
    return err == ESP_ERR_NVS_NOT_FOUND ? ESP_ERR_NOT_FOUND : err;
}

static esp_err_t write_blob(const char* name, const char* key, const void* data, size_t size)
{
    nvs_handle_t handle;
    ESP_RETURN_ON_ERROR(open_namespace(name, NVS_READWRITE, &handle), TAG, "open %s", name);

    esp_err_t err = nvs_set_blob(handle, key, data, size);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);
    ESP_RETURN_ON_ERROR(err, TAG, "write %s/%s", name, key);

    return ESP_OK;
}

// Reads a blob of at most capacity bytes; *size is its actual length.
static esp_err_t read_blob(const char* name, const char* key, void* data, size_t capacity, size_t* size)
{
    nvs_handle_t handle;
    esp_err_t err = open_namespace(name, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return err;
    }

    *size = 0;
    err = nvs_get_blob(handle, key, NULL, size);
    if (err == ESP_OK) {
        err = *size <= capacity ? nvs_get_blob(handle, key, data, size) : ESP_ERR_INVALID_SIZE;
    }

    nvs_close(handle);

    return err == ESP_ERR_NVS_NOT_FOUND ? ESP_ERR_NOT_FOUND : err;
}

esp_err_t storage_load_settings(storage_settings_t* out)
{
    settings_record_t record;
    size_t size = 0;
    const esp_err_t err = read_blob(SETTINGS_NAMESPACE, SETTINGS_KEY, &record, sizeof(record), &size);
    if (err != ESP_OK) {
        return err;
    }

    if (size < 1 || record.format != SETTINGS_FORMAT) {
        return ESP_ERR_INVALID_VERSION;
    }

    if (size != sizeof(record)) {
        return ESP_ERR_INVALID_SIZE;
    }

    out->brightness_percent = record.brightness_percent;

    return ESP_OK;
}

esp_err_t storage_save_settings(const storage_settings_t* settings)
{
    const settings_record_t record = {
        .format = SETTINGS_FORMAT,
        .brightness_percent = settings->brightness_percent,
    };

    return write_blob(SETTINGS_NAMESPACE, SETTINGS_KEY, &record, sizeof(record));
}

static void bsec_record_key(uint8_t profile, char* key, size_t size)
{
    snprintf(key, size, "state%u", profile);
}

esp_err_t storage_load_bsec_state(
    const storage_bsec_key_t* key,
    uint8_t* state,
    size_t capacity,
    size_t* length,
    uint8_t* accuracy)
{
    char name[NVS_KEY_NAME_MAX_SIZE];
    bsec_record_key(key->profile, name, sizeof(name));

    uint8_t* record = malloc(sizeof(bsec_header_t) + BSEC_STATE_MAX_BYTES);
    ESP_RETURN_ON_FALSE(record != NULL, ESP_ERR_NO_MEM, TAG, "record buffer");

    size_t size = 0;
    esp_err_t err = read_blob(BSEC_NAMESPACE, name, record, sizeof(bsec_header_t) + BSEC_STATE_MAX_BYTES, &size);
    if (err == ESP_OK && size < sizeof(bsec_header_t)) {
        err = ESP_ERR_INVALID_SIZE;
    }

    if (err != ESP_OK) {
        free(record);

        return err;
    }

    bsec_header_t header;
    memcpy(&header, record, sizeof(header));
    const uint8_t* saved = record + sizeof(header);

    if (header.format != BSEC_FORMAT || header.profile != key->profile || header.bsec_version != key->bsec_version ||
        header.config_crc != key->config_crc) {
        err = ESP_ERR_INVALID_VERSION;
    } else if (header.length != size - sizeof(header)) {
        err = ESP_ERR_INVALID_SIZE;
    } else if (esp_rom_crc32_le(0, saved, header.length) != header.state_crc) {
        err = ESP_ERR_INVALID_CRC;
    } else if (header.length > capacity) {
        err = ESP_ERR_INVALID_SIZE;
    } else {
        memcpy(state, saved, header.length);
        *length = header.length;
        *accuracy = header.accuracy;
    }

    free(record);

    return err;
}

esp_err_t storage_save_bsec_state(const storage_bsec_key_t* key, const uint8_t* state, size_t length, uint8_t accuracy)
{
    ESP_RETURN_ON_FALSE(length > 0 && length <= BSEC_STATE_MAX_BYTES, ESP_ERR_INVALID_SIZE, TAG, "state length");

    char name[NVS_KEY_NAME_MAX_SIZE];
    bsec_record_key(key->profile, name, sizeof(name));

    const bsec_header_t header = {
        .format = BSEC_FORMAT,
        .profile = key->profile,
        .accuracy = accuracy,
        .bsec_version = key->bsec_version,
        .config_crc = key->config_crc,
        .length = length,
        .state_crc = esp_rom_crc32_le(0, state, length),
    };

    uint8_t* record = malloc(sizeof(header) + length);
    ESP_RETURN_ON_FALSE(record != NULL, ESP_ERR_NO_MEM, TAG, "record buffer");
    memcpy(record, &header, sizeof(header));
    memcpy(record + sizeof(header), state, length);

    // One blob, so NVS replaces the old record only once the new one is complete.
    const esp_err_t err = write_blob(BSEC_NAMESPACE, name, record, sizeof(header) + length);
    free(record);

    return err;
}
