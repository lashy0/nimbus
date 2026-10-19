#include "board.h"

#include "board_pins.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_check.h"

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

static esp_err_t release_button_pin(gpio_num_t pin)
{
    ESP_RETURN_ON_ERROR(rtc_gpio_hold_dis(pin), TAG, "button RTC release");
    ESP_RETURN_ON_ERROR(rtc_gpio_deinit(pin), TAG, "button RTC mux");

    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_INPUT,
        // GPIO35 is input-only without internal pulls; the board has an external pull-up.
        .pull_up_en = pin == BOARD_PIN_BUTTON_PREV ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    return gpio_config(&config);
}

esp_err_t board_init_safe_gpio(void)
{
    ESP_RETURN_ON_ERROR(restore_low_output(BOARD_PIN_BACKLIGHT), TAG, "backlight restore");
    ESP_RETURN_ON_ERROR(restore_low_output(BOARD_PIN_ADC_ENABLE), TAG, "ADC_EN restore");

    ESP_RETURN_ON_ERROR(gpio_set_level(BOARD_PIN_PANEL_RESET, 1), TAG, "RST HIGH");
    ESP_RETURN_ON_ERROR(gpio_set_direction(BOARD_PIN_PANEL_RESET, GPIO_MODE_OUTPUT), TAG, "RST output");
    ESP_RETURN_ON_ERROR(gpio_hold_dis(BOARD_PIN_PANEL_RESET), TAG, "RST release");
    gpio_deep_sleep_hold_dis();

    ESP_RETURN_ON_ERROR(release_button_pin(BOARD_PIN_BUTTON_PREV), TAG, "PREV input");

    return release_button_pin(BOARD_PIN_BUTTON_NEXT);
}
