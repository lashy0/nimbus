#include "power_manager_internal.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char* TAG = "power_mgr";

#define DISPLAY_ACTIVE_BRIGHTNESS_PCT 60
#define BRIGHTNESS_MIN_PCT 5
#define BRIGHTNESS_MAX_PCT 100
#define BRIGHTNESS_NVS_NAMESPACE "app_settings"
#define BRIGHTNESS_NVS_KEY "brightness_pct"

static uint8_t s_active_brightness_pct = DISPLAY_ACTIVE_BRIGHTNESS_PCT;
static const uint8_t brightness_presets[] = {20, 40, 60, 80, 100};

static uint8_t brightness_clamp(uint8_t brightness_percent)
{
    if (brightness_percent < BRIGHTNESS_MIN_PCT) {
        return BRIGHTNESS_MIN_PCT;
    }
    if (brightness_percent > BRIGHTNESS_MAX_PCT) {
        return BRIGHTNESS_MAX_PCT;
    }
    return brightness_percent;
}

static void brightness_try_init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_OK || ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND || ret == ESP_ERR_INVALID_STATE) {
        return;
    }

    ESP_LOGW(TAG, "NVS init for brightness failed: %s", esp_err_to_name(ret));
}

static void brightness_load_from_nvs(void)
{
    nvs_handle_t nvs = 0;
    if (nvs_open(BRIGHTNESS_NVS_NAMESPACE, NVS_READONLY, &nvs) != ESP_OK) {
        return;
    }

    uint8_t brightness = 0;
    esp_err_t ret = nvs_get_u8(nvs, BRIGHTNESS_NVS_KEY, &brightness);
    nvs_close(nvs);
    if (ret == ESP_OK) {
        s_active_brightness_pct = brightness_clamp(brightness);
    }
}

static void brightness_save_to_nvs(uint8_t brightness)
{
    nvs_handle_t nvs = 0;
    if (nvs_open(BRIGHTNESS_NVS_NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK) {
        return;
    }

    esp_err_t ret = nvs_set_u8(nvs, BRIGHTNESS_NVS_KEY, brightness);
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs);
    }
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to persist brightness: %s", esp_err_to_name(ret));
    }

    nvs_close(nvs);
}

void pm_brightness_init(void)
{
    brightness_try_init_nvs();
    brightness_load_from_nvs();
}

void pm_brightness_apply_current(void)
{
    if (s_pm_config.bl_handle && !s_is_monitoring) {
        backlight_set_brightness(s_pm_config.bl_handle, s_active_brightness_pct);
    }
}

esp_err_t power_manager_set_active_brightness(uint8_t brightness_percent, bool persist)
{
    if (brightness_percent < BRIGHTNESS_MIN_PCT || brightness_percent > BRIGHTNESS_MAX_PCT) {
        return ESP_ERR_INVALID_ARG;
    }

    s_active_brightness_pct = brightness_percent;
    pm_brightness_apply_current();

    if (persist) {
        brightness_try_init_nvs();
        brightness_save_to_nvs(brightness_percent);
    }

    return ESP_OK;
}

esp_err_t power_manager_step_active_brightness(void)
{
    const size_t preset_count = sizeof(brightness_presets) / sizeof(brightness_presets[0]);
    uint8_t next = brightness_presets[0];

    for (size_t i = 0; i < preset_count; i++) {
        if (brightness_presets[i] > s_active_brightness_pct) {
            next = brightness_presets[i];
            return power_manager_set_active_brightness(next, true);
        }
    }

    return power_manager_set_active_brightness(next, true);
}

uint8_t power_manager_get_active_brightness(void)
{
    return s_active_brightness_pct;
}
