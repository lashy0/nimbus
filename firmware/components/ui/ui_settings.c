#include "ui_settings.h"

#include "esp_check.h"
#include "nvs.h"

#define NVS_NAMESPACE "ui"
#define NVS_KEY_BRIGHTNESS "brightness"

static const char* TAG = "ui_settings";

esp_err_t ui_settings_load_brightness(uint8_t* out)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs);
    // A read-only open fails until the first save creates the namespace.
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_ERR_NOT_FOUND;
    }
    ESP_RETURN_ON_ERROR(err, TAG, "open");

    err = nvs_get_u8(nvs, NVS_KEY_BRIGHTNESS, out);
    nvs_close(nvs);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_ERR_NOT_FOUND;
    }
    ESP_RETURN_ON_ERROR(err, TAG, "read");

    return ESP_OK;
}

esp_err_t ui_settings_save_brightness(uint8_t percent)
{
    nvs_handle_t nvs;
    ESP_RETURN_ON_ERROR(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs), TAG, "open");

    esp_err_t err = nvs_set_u8(nvs, NVS_KEY_BRIGHTNESS, percent);
    if (err == ESP_OK) {
        err = nvs_commit(nvs);
    }
    nvs_close(nvs);
    ESP_RETURN_ON_ERROR(err, TAG, "write");

    return ESP_OK;
}
