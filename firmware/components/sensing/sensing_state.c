#include "sensing_state.h"

#include "esp_check.h"
#include "nvs.h"

#include "sensing_bsec.h"

#define NVS_NAMESPACE "sensing"
#define NVS_KEY "bsec_state"

static const char* TAG = "sensing_state";

esp_err_t sensing_state_load(void)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs);
    // A read-only open fails until the first save creates the namespace.
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_ERR_NOT_FOUND;
    }
    ESP_RETURN_ON_ERROR(err, TAG, "open");

    uint8_t state[BSEC_MAX_STATE_BLOB_SIZE];
    size_t size = sizeof(state);
    err = nvs_get_blob(nvs, NVS_KEY, state, &size);
    nvs_close(nvs);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_ERR_NOT_FOUND;
    }
    ESP_RETURN_ON_ERROR(err, TAG, "read");

    return sensing_bsec_set_state(state, size);
}

esp_err_t sensing_state_save(void)
{
    uint8_t state[BSEC_MAX_STATE_BLOB_SIZE];
    uint32_t size;
    ESP_RETURN_ON_ERROR(sensing_bsec_get_state(state, &size), TAG, "get state");

    nvs_handle_t nvs;
    ESP_RETURN_ON_ERROR(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs), TAG, "open");

    esp_err_t err = nvs_set_blob(nvs, NVS_KEY, state, size);
    if (err == ESP_OK) {
        err = nvs_commit(nvs);
    }
    nvs_close(nvs);
    ESP_RETURN_ON_ERROR(err, TAG, "write");

    return ESP_OK;
}
