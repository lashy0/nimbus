#include "power_manager_internal.h"

#include <string.h>

#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

static const char* TAG = "power_mgr";

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

static adc_oneshot_unit_handle_t battery_adc_handle = NULL;
static adc_cali_handle_t battery_cali_handle = NULL;
static bool battery_cali_enabled = false;
static bool battery_filter_valid = false;
static float battery_filtered_mv = 0.0f;
static float battery_prev_mv = 0.0f;
static bool battery_charging = false;
static uint8_t battery_charge_trend_count = 0;
static uint8_t battery_discharge_trend_count = 0;

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

static void battery_reset_trend_counters(void)
{
    battery_charge_trend_count = 0;
    battery_discharge_trend_count = 0;
}

static float battery_filter_apply(float batt_mv)
{
    if (!battery_filter_valid) {
        battery_filter_valid = true;
        battery_filtered_mv = batt_mv;
        battery_prev_mv = batt_mv;
    } else {
        battery_filtered_mv = battery_filtered_mv * (1.0f - BATTERY_FILTER_ALPHA) + batt_mv * BATTERY_FILTER_ALPHA;
    }

    return battery_filtered_mv;
}

static void battery_update_charging_trend(float delta_mv)
{
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
        battery_reset_trend_counters();
    }

    if (battery_charge_trend_count >= BATTERY_TREND_CONFIRM_SAMPLES) {
        battery_charging = true;
    } else if (battery_discharge_trend_count >= BATTERY_TREND_CONFIRM_SAMPLES) {
        battery_charging = false;
    }
}

static esp_err_t battery_read_pin_mv(int* out_pin_mv)
{
    if (!out_pin_mv) {
        return ESP_ERR_INVALID_ARG;
    }

    int raw = 0;
    esp_err_t ret = adc_oneshot_read(battery_adc_handle, BATTERY_ADC_CHANNEL, &raw);
    if (ret != ESP_OK) {
        return ret;
    }

    if (battery_cali_enabled) {
        return adc_cali_raw_to_voltage(battery_cali_handle, raw, out_pin_mv);
    }

    *out_pin_mv = (raw * 3300) / 4095;
    return ESP_OK;
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

void pm_battery_init(void)
{
    battery_monitor_init();
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

    int pin_mv = 0;
    ret = battery_read_pin_mv(&pin_mv);
    if (ret != ESP_OK) {
        return ret;
    }

    float batt_mv = ((float)pin_mv) * BATTERY_DIVIDER_RATIO;
    battery_filter_apply(batt_mv);
    float delta_mv = battery_filtered_mv - battery_prev_mv;
    battery_prev_mv = battery_filtered_mv;

    int mv_rounded = (int)(battery_filtered_mv + 0.5f);
    out_info->voltage_mv = (mv_rounded > 0) ? (uint16_t)mv_rounded : 0U;

    if (mv_rounded < BATTERY_VALID_MIN_MV || mv_rounded > BATTERY_VALID_MAX_MV) {
        battery_reset_trend_counters();
        out_info->percent = -1;
        out_info->charging = false;
        out_info->valid = false;
        return ESP_OK;
    }

    battery_update_charging_trend(delta_mv);

    out_info->percent = battery_percent_from_mv(mv_rounded);
    out_info->charging = battery_charging;
    out_info->valid = true;

    return ESP_OK;
}
