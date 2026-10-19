#include "board_backlight.h"

#include "board_pins.h"
#include "driver/ledc.h"
#include "esp_check.h"

#define BACKLIGHT_TIMER LEDC_TIMER_0
#define BACKLIGHT_CHANNEL LEDC_CHANNEL_0
#define BACKLIGHT_MODE LEDC_LOW_SPEED_MODE
#define BACKLIGHT_RESOLUTION LEDC_TIMER_10_BIT
#define BACKLIGHT_FREQUENCY_HZ 5000

static const char* TAG = "board_backlight";

esp_err_t board_backlight_init(void)
{
    const ledc_timer_config_t timer_config = {
        .speed_mode = BACKLIGHT_MODE,
        .duty_resolution = BACKLIGHT_RESOLUTION,
        .timer_num = BACKLIGHT_TIMER,
        .freq_hz = BACKLIGHT_FREQUENCY_HZ,
        .clk_cfg = LEDC_USE_APB_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_config), TAG, "LEDC timer");

    const ledc_channel_config_t channel_config = {
        .gpio_num = BOARD_PIN_BACKLIGHT,
        .speed_mode = BACKLIGHT_MODE,
        .channel = BACKLIGHT_CHANNEL,
        .timer_sel = BACKLIGHT_TIMER,
        .duty = 0,
        .hpoint = 0,
    };

    return ledc_channel_config(&channel_config);
}

esp_err_t board_backlight_set(void* ctx, uint8_t percent)
{
    ESP_RETURN_ON_FALSE(percent <= 100, ESP_ERR_INVALID_ARG, TAG, "percent");

    const uint32_t duty = ((1U << BACKLIGHT_RESOLUTION) - 1) * percent / 100;
    ESP_RETURN_ON_ERROR(ledc_set_duty(BACKLIGHT_MODE, BACKLIGHT_CHANNEL, duty), TAG, "duty");

    return ledc_update_duty(BACKLIGHT_MODE, BACKLIGHT_CHANNEL);
}
