#include "board_battery.h"

#include <stdlib.h>

#include "board_pins.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#define BATTERY_UNIT ADC_UNIT_1
#define BATTERY_CHANNEL ADC_CHANNEL_6 // GPIO34
#define BATTERY_ATTEN ADC_ATTEN_DB_12
#define BATTERY_DIVIDER 2
#define SAMPLE_COUNT 32
#define SETTLE_US 2000 // After ADC_EN goes HIGH; the value used in the stage 1 series.
#define SAMPLE_GAP_US 100

static const char* TAG = "board_battery";

static adc_oneshot_unit_handle_t s_unit;
static adc_cali_handle_t s_calibration;

esp_err_t board_battery_init(void)
{
    ESP_RETURN_ON_FALSE(s_unit == NULL, ESP_ERR_INVALID_STATE, TAG, "already initialized");

    adc_cali_line_fitting_efuse_val_t source;
    ESP_RETURN_ON_ERROR(adc_cali_scheme_line_fitting_check_efuse(&source), TAG, "eFuse check");
    ESP_RETURN_ON_FALSE(
        source != ADC_CALI_LINE_FITTING_EFUSE_VAL_DEFAULT_VREF,
        ESP_ERR_NOT_SUPPORTED,
        TAG,
        "no ADC calibration in eFuse");

    const adc_cali_line_fitting_config_t calibration_config = {
        .unit_id = BATTERY_UNIT,
        .atten = BATTERY_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };
    adc_cali_handle_t calibration = NULL;
    adc_oneshot_unit_handle_t unit = NULL;
    esp_err_t ret = ESP_OK;

    ESP_GOTO_ON_ERROR(adc_cali_create_scheme_line_fitting(&calibration_config, &calibration), fail, TAG, "calibration");

    const adc_oneshot_unit_init_cfg_t unit_config = {.unit_id = BATTERY_UNIT};
    ESP_GOTO_ON_ERROR(adc_oneshot_new_unit(&unit_config, &unit), fail, TAG, "ADC1");

    const adc_oneshot_chan_cfg_t channel_config = {
        .atten = BATTERY_ATTEN,
        .bitwidth = ADC_BITWIDTH_12,
    };

    ESP_GOTO_ON_ERROR(adc_oneshot_config_channel(unit, BATTERY_CHANNEL, &channel_config), fail, TAG, "channel");

    // Readers see a unit only after its channel and calibration are both ready.
    s_calibration = calibration;
    s_unit = unit;

    return ESP_OK;

fail:
    if (unit != NULL) {
        const esp_err_t err = adc_oneshot_del_unit(unit);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "ADC cleanup failed: %s", esp_err_to_name(err));
        }
    }

    if (calibration != NULL) {
        const esp_err_t err = adc_cali_delete_scheme_line_fitting(calibration);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "calibration cleanup failed: %s", esp_err_to_name(err));
        }
    }

    return ret;
}

static int compare_int(const void* a, const void* b)
{
    const int left = *(const int*)a;
    const int right = *(const int*)b;

    return (left > right) - (left < right);
}

static esp_err_t read_samples(int* mv)
{
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        int raw = 0;
        ESP_RETURN_ON_ERROR(adc_oneshot_read(s_unit, BATTERY_CHANNEL, &raw), TAG, "read");
        ESP_RETURN_ON_ERROR(adc_cali_raw_to_voltage(s_calibration, raw, &mv[i]), TAG, "calibrate");
        esp_rom_delay_us(SAMPLE_GAP_US);
    }

    return ESP_OK;
}

esp_err_t board_battery_read(void* ctx, uint32_t* battery_mv)
{
    ESP_RETURN_ON_FALSE(s_unit != NULL, ESP_ERR_INVALID_STATE, TAG, "not initialized");

    int mv[SAMPLE_COUNT];
    esp_err_t err = gpio_set_level(BOARD_PIN_ADC_ENABLE, 1);
    if (err == ESP_OK) {
        esp_rom_delay_us(SETTLE_US);
        err = read_samples(mv);
    }

    // ADC_EN must not stay HIGH after a failed read either.
    const esp_err_t disable_err = gpio_set_level(BOARD_PIN_ADC_ENABLE, 0);
    ESP_RETURN_ON_ERROR(err, TAG, "series");
    ESP_RETURN_ON_ERROR(disable_err, TAG, "ADC_EN LOW");

    qsort(mv, SAMPLE_COUNT, sizeof(mv[0]), compare_int);
    *battery_mv = (uint32_t)(mv[SAMPLE_COUNT / 2 - 1] + mv[SAMPLE_COUNT / 2]) / 2 * BATTERY_DIVIDER;

    return ESP_OK;
}
