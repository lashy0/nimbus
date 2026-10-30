#include "power_battery.h"

#include <stdlib.h>

#include "driver/gpio.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_rom_sys.h"

#define ATTENUATION ADC_ATTEN_DB_12
#define DIVIDER 2
#define SAMPLE_COUNT 32
#define SAMPLE_GAP_US 100
#define SETTLE_US 2000 // After ADC_EN goes HIGH.

static const char* TAG = "power_battery";

static adc_oneshot_unit_handle_t s_unit;
static adc_cali_handle_t s_calibration;
static adc_channel_t s_channel;
static gpio_num_t s_adc_enable;

static esp_err_t init_adc(adc_unit_t unit_id)
{
    const adc_cali_line_fitting_config_t calibration_config = {
        .unit_id = unit_id,
        .atten = ATTENUATION,
        .bitwidth = ADC_BITWIDTH_12,
    };
    ESP_RETURN_ON_ERROR(adc_cali_create_scheme_line_fitting(&calibration_config, &s_calibration), TAG, "calibration");

    const adc_oneshot_unit_init_cfg_t unit_config = {.unit_id = unit_id};
    const adc_oneshot_chan_cfg_t channel_config = {.atten = ATTENUATION, .bitwidth = ADC_BITWIDTH_12};

    esp_err_t err = adc_oneshot_new_unit(&unit_config, &s_unit);
    if (err == ESP_OK) {
        err = adc_oneshot_config_channel(s_unit, s_channel, &channel_config);
    }

    if (err != ESP_OK) {
        if (s_unit != NULL) {
            adc_oneshot_del_unit(s_unit);
            s_unit = NULL;
        }
        adc_cali_delete_scheme_line_fitting(s_calibration);
        s_calibration = NULL;
    }
    ESP_RETURN_ON_ERROR(err, TAG, "ADC");

    return ESP_OK;
}

esp_err_t power_battery_init(gpio_num_t battery, gpio_num_t adc_enable)
{
    adc_unit_t unit_id;
    ESP_RETURN_ON_ERROR(adc_oneshot_io_to_channel(battery, &unit_id, &s_channel), TAG, "GPIO %d", battery);
    // ADC2 is shared with Wi-Fi.
    ESP_RETURN_ON_FALSE(unit_id == ADC_UNIT_1, ESP_ERR_INVALID_ARG, TAG, "GPIO %d is not on ADC1", battery);

    adc_cali_line_fitting_efuse_val_t source;
    ESP_RETURN_ON_ERROR(adc_cali_scheme_line_fitting_check_efuse(&source), TAG, "eFuse");
    ESP_RETURN_ON_FALSE(
        source != ADC_CALI_LINE_FITTING_EFUSE_VAL_DEFAULT_VREF, ESP_ERR_NOT_SUPPORTED, TAG, "no ADC calibration");

    const gpio_config_t enable_config = {
        .pin_bit_mask = 1ULL << adc_enable,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_RETURN_ON_ERROR(gpio_set_level(adc_enable, 0), TAG, "ADC_EN LOW");
    ESP_RETURN_ON_ERROR(gpio_config(&enable_config), TAG, "ADC_EN");
    s_adc_enable = adc_enable;

    return init_adc(unit_id);
}

static int compare_int(const void* a, const void* b)
{
    const int left = *(const int*)a;
    const int right = *(const int*)b;

    return (left > right) - (left < right);
}

static esp_err_t read_series(int* mv)
{
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        int raw;
        ESP_RETURN_ON_ERROR(adc_oneshot_read(s_unit, s_channel, &raw), TAG, "read");
        ESP_RETURN_ON_ERROR(adc_cali_raw_to_voltage(s_calibration, raw, &mv[i]), TAG, "calibrate");
        esp_rom_delay_us(SAMPLE_GAP_US);
    }

    return ESP_OK;
}

esp_err_t power_battery_read(uint32_t* out_mv)
{
    int mv[SAMPLE_COUNT];
    esp_err_t err = gpio_set_level(s_adc_enable, 1);
    if (err == ESP_OK) {
        esp_rom_delay_us(SETTLE_US);
        err = read_series(mv);
    }

    // Also after a failed series.
    const esp_err_t disable_err = gpio_set_level(s_adc_enable, 0);
    ESP_RETURN_ON_ERROR(err, TAG, "series");
    ESP_RETURN_ON_ERROR(disable_err, TAG, "ADC_EN LOW");

    qsort(mv, SAMPLE_COUNT, sizeof(mv[0]), compare_int);
    *out_mv = (uint32_t)(mv[SAMPLE_COUNT / 2 - 1] + mv[SAMPLE_COUNT / 2]) / 2 * DIVIDER;

    return ESP_OK;
}
