#include "board.h"

#include "board_gpio.h"
#include "board_pins.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_check.h"

static const char* TAG = "board";

static esp_err_t release_button_pin(board_button_t button)
{
    const gpio_num_t pin = board_button_pin(button);
    ESP_RETURN_ON_ERROR(rtc_gpio_hold_dis(pin), TAG, "button RTC release");
    ESP_RETURN_ON_ERROR(rtc_gpio_deinit(pin), TAG, "button RTC mux");

    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = board_button_internal_pull_up(button) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    return gpio_config(&config);
}

esp_err_t board_init_safe_gpio(void)
{
    ESP_RETURN_ON_ERROR(board_gpio_release_low(BOARD_PIN_BACKLIGHT), TAG, "backlight restore");
    ESP_RETURN_ON_ERROR(board_gpio_release_low(BOARD_PIN_ADC_ENABLE), TAG, "ADC_EN restore");

    ESP_RETURN_ON_ERROR(gpio_set_level(BOARD_PIN_PANEL_RESET, 1), TAG, "RST HIGH");
    ESP_RETURN_ON_ERROR(gpio_set_direction(BOARD_PIN_PANEL_RESET, GPIO_MODE_OUTPUT), TAG, "RST output");
    ESP_RETURN_ON_ERROR(gpio_hold_dis(BOARD_PIN_PANEL_RESET), TAG, "RST release");
    gpio_deep_sleep_hold_dis();

    for (unsigned i = 0; i < BOARD_BUTTON_COUNT; ++i) {
        ESP_RETURN_ON_ERROR(release_button_pin((board_button_t)i), TAG, "button %u input", i);
    }

    return ESP_OK;
}
