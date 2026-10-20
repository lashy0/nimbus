#include "board_gpio.h"

#include "driver/rtc_io.h"
#include "esp_check.h"

static const char* TAG = "board_gpio";

esp_err_t board_gpio_hold_low(gpio_num_t pin)
{
    ESP_RETURN_ON_ERROR(rtc_gpio_init(pin), TAG, "RTC mux");
    ESP_RETURN_ON_ERROR(rtc_gpio_set_level(pin, 0), TAG, "RTC LOW");
    ESP_RETURN_ON_ERROR(rtc_gpio_set_direction(pin, RTC_GPIO_MODE_OUTPUT_ONLY), TAG, "RTC output");
    ESP_RETURN_ON_ERROR(rtc_gpio_pullup_dis(pin), TAG, "pull-up off");
    ESP_RETURN_ON_ERROR(rtc_gpio_pulldown_dis(pin), TAG, "pull-down off");

    return rtc_gpio_hold_en(pin);
}

esp_err_t board_gpio_release_low(gpio_num_t pin)
{
    ESP_RETURN_ON_ERROR(rtc_gpio_init(pin), TAG, "RTC mux");
    ESP_RETURN_ON_ERROR(rtc_gpio_set_level(pin, 0), TAG, "RTC LOW");
    ESP_RETURN_ON_ERROR(rtc_gpio_set_direction(pin, RTC_GPIO_MODE_OUTPUT_ONLY), TAG, "RTC output");
    ESP_RETURN_ON_ERROR(rtc_gpio_hold_dis(pin), TAG, "RTC release");
    ESP_RETURN_ON_ERROR(gpio_set_level(pin, 0), TAG, "digital LOW");
    ESP_RETURN_ON_ERROR(gpio_set_direction(pin, GPIO_MODE_OUTPUT), TAG, "digital output");

    return rtc_gpio_deinit(pin);
}
