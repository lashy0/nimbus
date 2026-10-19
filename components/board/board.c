#include "board.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

enum {
    BACKLIGHT = GPIO_NUM_4,
    PANEL_RESET = GPIO_NUM_23,
    ADC_ENABLE = GPIO_NUM_14,
    BUTTON_PREV = GPIO_NUM_0,
    BUTTON_NEXT = GPIO_NUM_35,
};

static const char* TAG = "board";

static esp_err_t restore_low_output(gpio_num_t pin)
{
    ESP_RETURN_ON_ERROR(rtc_gpio_init(pin), TAG, "RTC mux");
    ESP_RETURN_ON_ERROR(rtc_gpio_set_level(pin, 0), TAG, "RTC LOW");
    ESP_RETURN_ON_ERROR(rtc_gpio_set_direction(pin, RTC_GPIO_MODE_OUTPUT_ONLY), TAG, "RTC output");
    ESP_RETURN_ON_ERROR(rtc_gpio_hold_dis(pin), TAG, "RTC release");
    ESP_RETURN_ON_ERROR(gpio_set_level(pin, 0), TAG, "digital LOW");
    ESP_RETURN_ON_ERROR(gpio_set_direction(pin, GPIO_MODE_OUTPUT), TAG, "digital output");
    return rtc_gpio_deinit(pin);
}

esp_err_t board_init_safe_gpio(void)
{
    ESP_RETURN_ON_ERROR(restore_low_output(BACKLIGHT), TAG, "backlight restore");
    ESP_RETURN_ON_ERROR(restore_low_output(ADC_ENABLE), TAG, "ADC_EN restore");
    ESP_RETURN_ON_ERROR(gpio_set_level(PANEL_RESET, 1), TAG, "RST HIGH");
    ESP_RETURN_ON_ERROR(gpio_set_direction(PANEL_RESET, GPIO_MODE_OUTPUT), TAG, "RST output");
    ESP_RETURN_ON_ERROR(gpio_hold_dis(PANEL_RESET), TAG, "RST release");
    gpio_deep_sleep_hold_dis();

    ESP_RETURN_ON_ERROR(rtc_gpio_hold_dis(BUTTON_PREV), TAG, "PREV release");
    ESP_RETURN_ON_ERROR(rtc_gpio_hold_dis(BUTTON_NEXT), TAG, "NEXT release");
    ESP_RETURN_ON_ERROR(rtc_gpio_deinit(BUTTON_PREV), TAG, "PREV mux");
    ESP_RETURN_ON_ERROR(rtc_gpio_deinit(BUTTON_NEXT), TAG, "NEXT mux");
    const gpio_config_t prev_config = {
        .pin_bit_mask = 1ULL << BUTTON_PREV,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    const gpio_config_t next_config = {
        .pin_bit_mask = 1ULL << BUTTON_NEXT,
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&prev_config), TAG, "PREV input");
    return gpio_config(&next_config);
}

esp_err_t board_adc_init(board_adc_t* adc)
{
    ESP_RETURN_ON_FALSE(adc != NULL && adc->unit == NULL, ESP_ERR_INVALID_ARG, TAG, "ADC context");
    const adc_oneshot_unit_init_cfg_t unit_config = {.unit_id = ADC_UNIT_1};
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit_config, &adc->unit), TAG, "ADC1 create");
    const adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };
    esp_err_t err = adc_oneshot_config_channel(adc->unit, ADC_CHANNEL_6, &channel_config);
    if (err != ESP_OK) {
        adc_oneshot_del_unit(adc->unit);
        adc->unit = NULL;
        return err;
    }

    adc_cali_line_fitting_efuse_val_t source;
    err = adc_cali_scheme_line_fitting_check_efuse(&source);
    if (err == ESP_OK && source != ADC_CALI_LINE_FITTING_EFUSE_VAL_DEFAULT_VREF) {
        const adc_cali_line_fitting_config_t calibration_config = {
            .unit_id = ADC_UNIT_1,
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_12,
        };
        err = adc_cali_create_scheme_line_fitting(&calibration_config, &adc->calibration);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "ADC calibration=eFuse source=%d", source);
            return ESP_OK;
        }
    }
    ESP_LOGW(TAG, "ADC calibration unavailable (%s); raw only, no assumed Vref", esp_err_to_name(err));
    return ESP_OK; // ADC raw remains available independently of calibration.
}

static void sort_samples(int* samples)
{
    for (unsigned i = 1; i < BOARD_ADC_SAMPLE_COUNT; ++i) {
        int value = samples[i];
        unsigned j = i;
        while (j > 0 && samples[j - 1] > value) {
            samples[j] = samples[j - 1];
            --j;
        }
        samples[j] = value;
    }
}

esp_err_t board_adc_measure(board_adc_t* adc, bool enable, unsigned settle_us, board_adc_reading_t* out)
{
    esp_err_t err = gpio_set_level(ADC_ENABLE, 0);
    if (err != ESP_OK) {
        return err;
    }
    if (adc == NULL || adc->unit == NULL || out == NULL || settle_us > 100000) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(out, 0, sizeof(*out));
    int raw[BOARD_ADC_SAMPLE_COUNT];
    int mv[BOARD_ADC_SAMPLE_COUNT];
    bool calibrated = adc->calibration != NULL;
    const int64_t started = esp_timer_get_time();
    err = gpio_set_level(ADC_ENABLE, enable ? 1 : 0);
    if (err != ESP_OK) {
        goto cleanup;
    }
    esp_rom_delay_us(settle_us);
    for (unsigned i = 0; i < BOARD_ADC_SAMPLE_COUNT; ++i) {
        err = adc_oneshot_read(adc->unit, ADC_CHANNEL_6, &raw[i]);
        if (err != ESP_OK) {
            goto cleanup;
        }
        ++out->samples;
        if (raw[i] == 0 || raw[i] == 4095) {
            ++out->clipped_samples;
        }
        if (calibrated && adc_cali_raw_to_voltage(adc->calibration, raw[i], &mv[i]) != ESP_OK) {
            calibrated = false;
        }
        esp_rom_delay_us(1000);
    }
    sort_samples(raw);
    out->raw_min = raw[0];
    out->raw_median = (raw[15] + raw[16]) / 2;
    out->raw_max = raw[31];
    out->calibrated = calibrated;
    if (calibrated) {
        sort_samples(mv);
        out->input_mv_min = mv[0];
        out->input_mv_median = (mv[15] + mv[16]) / 2;
        out->input_mv_max = mv[31];
    }
cleanup:
    {
        const esp_err_t disable_err = gpio_set_level(ADC_ENABLE, 0);
        out->duration_us = esp_timer_get_time() - started;
        return err == ESP_OK ? disable_err : err;
    }
}

uint32_t board_buttons_pressed(void)
{
    return (gpio_get_level(BUTTON_PREV) == 0 ? 1U : 0U) |
           (gpio_get_level(BUTTON_NEXT) == 0 ? 2U : 0U);
}

static esp_err_t hold_rtc_low(gpio_num_t pin)
{
    ESP_RETURN_ON_ERROR(rtc_gpio_init(pin), TAG, "sleep RTC mux");
    ESP_RETURN_ON_ERROR(rtc_gpio_set_level(pin, 0), TAG, "sleep RTC LOW");
    ESP_RETURN_ON_ERROR(rtc_gpio_set_direction(pin, RTC_GPIO_MODE_OUTPUT_ONLY), TAG, "sleep RTC output");
    ESP_RETURN_ON_ERROR(rtc_gpio_pullup_dis(pin), TAG, "sleep pull-up off");
    ESP_RETURN_ON_ERROR(rtc_gpio_pulldown_dis(pin), TAG, "sleep pull-down off");
    return rtc_gpio_hold_en(pin);
}

esp_err_t board_prepare_deep_sleep(bool next_only, uint32_t* wake_buttons)
{
    ESP_RETURN_ON_FALSE(wake_buttons != NULL, ESP_ERR_INVALID_ARG, TAG, "wake result");
    ESP_RETURN_ON_ERROR(hold_rtc_low(BACKLIGHT), TAG, "hold backlight");
    ESP_RETURN_ON_ERROR(hold_rtc_low(ADC_ENABLE), TAG, "hold ADC_EN");
    ESP_RETURN_ON_ERROR(gpio_set_level(PANEL_RESET, 0), TAG, "RST LOW");
    esp_rom_delay_us(1000);
    ESP_RETURN_ON_ERROR(gpio_set_level(PANEL_RESET, 1), TAG, "RST HIGH");
    vTaskDelay(pdMS_TO_TICKS(120) + 1); // At least 120 ms even near a tick boundary.
    ESP_RETURN_ON_ERROR(gpio_hold_en(PANEL_RESET), TAG, "hold RST");
    gpio_deep_sleep_hold_en();

    // Diagnostic start-up path only; app later supplies confirmed-release events.
    const int64_t deadline = esp_timer_get_time() + 3000000;
    int64_t released_since[2] = {0};
    while (esp_timer_get_time() < deadline) {
        const int64_t now = esp_timer_get_time();
        const uint32_t pressed = board_buttons_pressed();
        for (unsigned i = 0; i < 2; ++i) {
            if ((pressed & (1U << i)) != 0) {
                released_since[i] = 0;
            } else if (released_since[i] == 0) {
                released_since[i] = now;
            }
        }
        if (released_since[0] != 0 && released_since[1] != 0 &&
            now - released_since[0] >= 50000 && now - released_since[1] >= 50000) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    ESP_RETURN_ON_ERROR(esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL), TAG, "clear wake");
    uint32_t released = 0;
    const int64_t now = esp_timer_get_time();
    const uint32_t pressed = board_buttons_pressed();
    for (unsigned i = 0; i < 2; ++i) {
        if ((pressed & (1U << i)) == 0 && released_since[i] != 0 && now - released_since[i] >= 50000) {
            released |= 1U << i;
        }
    }
    *wake_buttons = released & (next_only ? 2U : 3U);
    if ((*wake_buttons & 1U) != 0) {
        ESP_RETURN_ON_ERROR(esp_sleep_enable_ext0_wakeup(BUTTON_PREV, 0), TAG, "ext0 PREV");
    }
    if ((*wake_buttons & 2U) != 0) {
        ESP_RETURN_ON_ERROR(esp_sleep_enable_ext1_wakeup_io(1ULL << BUTTON_NEXT, ESP_EXT1_WAKEUP_ALL_LOW),
                            TAG, "ext1 NEXT");
    }
    return ESP_OK;
}
