#include "board_diagnostics.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "driver/uart.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_system.h"

static const char* TAG = "bringup";

static RTC_NOINIT_ATTR struct {
    uint32_t magic;
    uint32_t boots;
    uint32_t boots_inverse;
} rtc_probe;

static void log_adc(board_adc_t* adc)
{
    for (unsigned enabled = 0; enabled < 2; ++enabled) {
        board_adc_reading_t reading;
        const esp_err_t err = board_adc_measure(adc, enabled != 0, 2000, &reading);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "ADC_EN=%u read failed: %s; ADC_EN returned LOW", enabled, esp_err_to_name(err));
            continue;
        }
        ESP_LOGI(TAG, "ADC_EN=%u settle_us=2000 n=%u raw=%d/%d/%d clipped=%u duration_us=%" PRId64,
                 enabled, reading.samples, reading.raw_min, reading.raw_median, reading.raw_max,
                 reading.clipped_samples, reading.duration_us);
        if (reading.calibrated) {
            ESP_LOGI(TAG, "ADC_EN=%u input_mV=%d/%d/%d divider_x2_mV_assumed=%d (not validated battery/SOC)",
                     enabled, reading.input_mv_min, reading.input_mv_median, reading.input_mv_max,
                     reading.input_mv_median * 2);
        } else {
            ESP_LOGW(TAG, "ADC_EN=%u input_mV=UNKNOWN", enabled);
        }
    }
}

static void sleep_now(bool next_only)
{
    uint32_t wake_buttons = 0;
    const esp_err_t err = board_prepare_deep_sleep(next_only, &wake_buttons);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "sleep preparation failed: %s; pin holds not confirmed; terminal sleep without wake",
                 esp_err_to_name(err));
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
        wake_buttons = 0;
    } else {
        ESP_LOGI(TAG, "DEEP_SLEEP panel=HW_RESET GPIO4/14=RTC_LOW GPIO23=HOLD_HIGH wake_mask=%" PRIu32
                     " (PREV=1 NEXT=2); no timer; current not measured", wake_buttons);
    }
    uart_wait_tx_done(UART_NUM_0, pdMS_TO_TICKS(1000));
    esp_deep_sleep_start();
}

void board_diagnostics_run(void)
{
    const bool retained = rtc_probe.magic == 0x4e494d32 && rtc_probe.boots_inverse == ~rtc_probe.boots;
    const uint32_t previous_boots = retained ? rtc_probe.boots : 0;
    rtc_probe.magic = 0x4e494d32;
    rtc_probe.boots = previous_boots + 1;
    rtc_probe.boots_inverse = ~rtc_probe.boots;
    ESP_LOGI(TAG, "STAGE1 reset_reason=%d (POWERON=%d DEEPSLEEP=%d) wake_cause=%d rtc_retained=%s boots=%" PRIu32,
             esp_reset_reason(), ESP_RST_POWERON, ESP_RST_DEEPSLEEP, esp_sleep_get_wakeup_cause(),
             retained ? "yes" : "no", rtc_probe.boots);

    esp_err_t err = board_init_safe_gpio();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "safe GPIO init failed: %s; no peripheral start", esp_err_to_name(err));
        return;
    }
    board_adc_t adc = {0};
    err = board_adc_init(&adc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ADC init failed: %s; ADC commands unavailable", esp_err_to_name(err));
    } else {
        log_adc(&adc);
    }

    err = uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UART command input failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Commands: adc | buttons | sleep (D1) | sleep-next (D3) | help; finish with newline");
    ESP_LOGI(TAG, "Display stays dark. ADC input calibration is not an independent battery measurement.");
    char command[32];
    size_t length = 0;
    bool overflow = false;
    for (;;) {
        uint8_t byte;
        if (uart_read_bytes(UART_NUM_0, &byte, 1, portMAX_DELAY) != 1) {
            continue;
        }
        if (byte == '\r' || byte == '\n') {
            if (overflow) {
                ESP_LOGW(TAG, "Command too long, discarded");
            } else if (length != 0) {
                command[length] = '\0';
                if (strcmp(command, "adc") == 0) {
                    log_adc(&adc);
                } else if (strcmp(command, "buttons") == 0) {
                    ESP_LOGI(TAG, "pressed_mask=%" PRIu32 " (PREV=1 NEXT=2)", board_buttons_pressed());
                } else if (strcmp(command, "sleep") == 0 || strcmp(command, "sleep-next") == 0) {
                    sleep_now(strcmp(command, "sleep-next") == 0);
                } else {
                    ESP_LOGI(TAG, "Commands: adc | buttons | sleep | sleep-next | help");
                }
            }
            length = 0;
            overflow = false;
        } else if (byte >= 32 && byte < 127 && !overflow) {
            if (length + 1 < sizeof(command)) {
                command[length++] = (char)byte;
            } else {
                overflow = true;
            }
        }
    }
}
