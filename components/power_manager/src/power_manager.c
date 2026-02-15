#include "power_manager.h"
#include <string.h>
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bme680_sensor.h"
#include "ui.h"
#include "esp_lvgl_port.h"

static const char* TAG = "power_mgr";

#define WAKEUP_GPIO GPIO_NUM_0
#define BATTERY_ADC_EN_GPIO GPIO_NUM_14
#define BATTERY_ADC_UNIT ADC_UNIT_1
#define BATTERY_ADC_CHANNEL ADC_CHANNEL_6
#define BATTERY_ADC_ATTEN ADC_ATTEN_DB_12
#define BATTERY_DIVIDER_RATIO 2.0f
#define BATTERY_FILTER_ALPHA 0.2f
#define BATTERY_CHARGE_DELTA_MV 5.0f
#define BATTERY_DISCHARGE_DELTA_MV -5.0f
#define BATTERY_TREND_CONFIRM_SAMPLES 2U
#define BATTERY_VALID_MIN_MV 2800
#define BATTERY_VALID_MAX_MV 4350
#define DISPLAY_ACTIVE_BRIGHTNESS_PCT 60
#define BRIGHTNESS_MIN_PCT 5
#define BRIGHTNESS_MAX_PCT 100
#define BRIGHTNESS_NVS_NAMESPACE "app_settings"
#define BRIGHTNESS_NVS_KEY "brightness_pct"

static power_manager_config_t pm_config;
static bool is_monitoring = false;
static adc_oneshot_unit_handle_t battery_adc_handle = NULL;
static adc_cali_handle_t battery_cali_handle = NULL;
static bool battery_cali_enabled = false;
static bool battery_filter_valid = false;
static float battery_filtered_mv = 0.0f;
static float battery_prev_mv = 0.0f;
static bool battery_charging = false;
static uint8_t battery_charge_trend_count = 0;
static uint8_t battery_discharge_trend_count = 0;
static uint8_t active_brightness_pct = DISPLAY_ACTIVE_BRIGHTNESS_PCT;
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

static void brightness_apply_current(void)
{
    if (pm_config.bl_handle && !is_monitoring) {
        backlight_set_brightness(pm_config.bl_handle, active_brightness_pct);
    }
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
        active_brightness_pct = brightness_clamp(brightness);
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

static int battery_percent_from_mv(int mv)
{
    static const int mv_points[] = {3200, 3400, 3500, 3600, 3700, 3800, 3900, 4000, 4100, 4200};
    static const int pct_points[] = {0, 5, 10, 20, 35, 50, 65, 80, 92, 100};
    const int points_count = (int)(sizeof(mv_points) / sizeof(mv_points[0]));

    if (mv <= mv_points[0]) {
        return pct_points[0];
    }
    if (mv >= mv_points[points_count - 1]) {
        return pct_points[points_count - 1];
    }

    for (int i = 1; i < points_count; i++) {
        if (mv <= mv_points[i]) {
            int mv_low = mv_points[i - 1];
            int mv_high = mv_points[i];
            int pct_low = pct_points[i - 1];
            int pct_high = pct_points[i];
            int pct = pct_low + ((mv - mv_low) * (pct_high - pct_low)) / (mv_high - mv_low);
            return pct;
        }
    }

    return 100;
}

static bool battery_try_create_cali(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t* out_handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = unit,
        .chan = channel,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_config, out_handle) == ESP_OK) {
        return true;
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = unit,
        .atten = atten,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_line_fitting(&cali_config, out_handle) == ESP_OK) {
        return true;
    }
#endif

    return false;
}

static void battery_monitor_init(void)
{
    gpio_config_t adc_en_cfg = {
        .pin_bit_mask = (1ULL << BATTERY_ADC_EN_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&adc_en_cfg);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Battery ADC_EN GPIO config failed: %s", esp_err_to_name(ret));
        return;
    }

    ret = gpio_set_level(BATTERY_ADC_EN_GPIO, 1);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Battery ADC_EN set failed: %s", esp_err_to_name(ret));
        return;
    }

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = BATTERY_ADC_UNIT,
    };
    ret = adc_oneshot_new_unit(&unit_cfg, &battery_adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Battery ADC init failed: %s", esp_err_to_name(ret));
        return;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = BATTERY_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_oneshot_config_channel(battery_adc_handle, BATTERY_ADC_CHANNEL, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Battery ADC channel config failed: %s", esp_err_to_name(ret));
        return;
    }

    battery_cali_enabled = battery_try_create_cali(BATTERY_ADC_UNIT, BATTERY_ADC_CHANNEL, BATTERY_ADC_ATTEN, &battery_cali_handle);
    ESP_LOGI(TAG, "Battery monitor initialized (cali=%s)", battery_cali_enabled ? "on" : "off");
}

void power_manager_init(const power_manager_config_t* config)
{
    pm_config = *config;
    battery_monitor_init();
    brightness_try_init_nvs();
    brightness_load_from_nvs();
    brightness_apply_current();
}

void power_manager_check_wakeup_reason(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    switch (cause)
    {
        case ESP_SLEEP_WAKEUP_EXT0:
            ESP_LOGI(TAG, "Wakeup from button press");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            ESP_LOGI(TAG, "Wakeup from EXT1");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Wakeup from timer");
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            ESP_LOGI(TAG, "Normal startup");
            break;
    }
}

void power_manager_shutdown(void)
{
    ESP_LOGI(TAG, "Shutting down...");

    if (lvgl_port_lock(100)) {
        ui_show_no_charging();
        lvgl_port_unlock();
    }

    vTaskDelay(pdMS_TO_TICKS(500));

    if (pm_config.bl_handle) {
        backlight_set_brightness(pm_config.bl_handle, 0);
    }

    if (pm_config.panel_handle) {
        esp_lcd_panel_disp_on_off(pm_config.panel_handle, false);
    }

    // Ensure deep sleep wakeup sources are deterministic: button only.
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 0);
    bme680_sensor_deinit();

    ESP_LOGI(TAG, "Entering deep sleep. Press button to wake up.");

    esp_deep_sleep_start();
}

void power_manager_enter_monitoring(void)
{
    ESP_LOGI(TAG, "Entering monitoring mode...");
    is_monitoring = true;

    if (pm_config.bl_handle) {
        backlight_set_brightness(pm_config.bl_handle, 0);
    }

    if (pm_config.panel_handle) {
        esp_lcd_panel_disp_on_off(pm_config.panel_handle, false);
    }
}

void power_manager_exit_monitoring(void)
{
    ESP_LOGI(TAG, "Exiting monitoring mode...");
    is_monitoring = false;

    if (pm_config.panel_handle) {
        esp_lcd_panel_disp_on_off(pm_config.panel_handle, true);
    }

    if (pm_config.bl_handle) {
        backlight_set_brightness(pm_config.bl_handle, active_brightness_pct);
    }
}

bool power_manager_is_monitoring(void)
{
    return is_monitoring;
}

esp_err_t power_manager_set_active_brightness(uint8_t brightness_percent, bool persist)
{
    if (brightness_percent < BRIGHTNESS_MIN_PCT || brightness_percent > BRIGHTNESS_MAX_PCT) {
        return ESP_ERR_INVALID_ARG;
    }

    active_brightness_pct = brightness_percent;
    brightness_apply_current();

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
        if (brightness_presets[i] > active_brightness_pct) {
            next = brightness_presets[i];
            return power_manager_set_active_brightness(next, true);
        }
    }

    return power_manager_set_active_brightness(next, true);
}

uint8_t power_manager_get_active_brightness(void)
{
    return active_brightness_pct;
}

esp_err_t power_manager_read_battery(power_battery_info_t* out_info)
{
    if (!out_info) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(out_info, 0, sizeof(*out_info));
    if (!battery_adc_handle) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = gpio_set_level(BATTERY_ADC_EN_GPIO, 1);
    if (ret != ESP_OK) {
        return ret;
    }
    esp_rom_delay_us(2000);

    int raw = 0;
    ret = adc_oneshot_read(battery_adc_handle, BATTERY_ADC_CHANNEL, &raw);
    if (ret != ESP_OK) {
        return ret;
    }

    int pin_mv = 0;
    if (battery_cali_enabled) {
        ret = adc_cali_raw_to_voltage(battery_cali_handle, raw, &pin_mv);
        if (ret != ESP_OK) {
            return ret;
        }
    } else {
        pin_mv = (raw * 3300) / 4095;
    }

    float batt_mv = ((float)pin_mv) * BATTERY_DIVIDER_RATIO;
    if (!battery_filter_valid) {
        battery_filter_valid = true;
        battery_filtered_mv = batt_mv;
        battery_prev_mv = batt_mv;
    } else {
        battery_filtered_mv = battery_filtered_mv * (1.0f - BATTERY_FILTER_ALPHA) + batt_mv * BATTERY_FILTER_ALPHA;
    }

    float delta_mv = battery_filtered_mv - battery_prev_mv;
    battery_prev_mv = battery_filtered_mv;

    int mv_rounded = (int)(battery_filtered_mv + 0.5f);
    out_info->voltage_mv = (mv_rounded > 0) ? (uint16_t)mv_rounded : 0U;

    if (mv_rounded < BATTERY_VALID_MIN_MV || mv_rounded > BATTERY_VALID_MAX_MV) {
        battery_charge_trend_count = 0;
        battery_discharge_trend_count = 0;
        out_info->percent = -1;
        out_info->charging = false;
        out_info->valid = false;
        return ESP_OK;
    }

    if (delta_mv >= BATTERY_CHARGE_DELTA_MV) {
        if (battery_charge_trend_count < 255U) {
            battery_charge_trend_count++;
        }
        battery_discharge_trend_count = 0;
    } else if (delta_mv <= BATTERY_DISCHARGE_DELTA_MV) {
        if (battery_discharge_trend_count < 255U) {
            battery_discharge_trend_count++;
        }
        battery_charge_trend_count = 0;
    } else {
        battery_charge_trend_count = 0;
        battery_discharge_trend_count = 0;
    }

    if (battery_charge_trend_count >= BATTERY_TREND_CONFIRM_SAMPLES) {
        battery_charging = true;
    } else if (battery_discharge_trend_count >= BATTERY_TREND_CONFIRM_SAMPLES) {
        battery_charging = false;
    }

    out_info->percent = battery_percent_from_mv(mv_rounded);
    out_info->charging = battery_charging;
    out_info->valid = true;

    return ESP_OK;
}
