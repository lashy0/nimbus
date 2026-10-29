#include "display.h"

#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"

#define PIXEL_CLOCK_HZ (40 * 1000 * 1000)
#define TRANS_QUEUE_DEPTH 10
// The 135x240 glass sits inside the 240x320 ST7789 memory, starting at column 52, row 40.
#define X_GAP 52
#define Y_GAP 40

#define BACKLIGHT_TIMER LEDC_TIMER_0
#define BACKLIGHT_CHANNEL LEDC_CHANNEL_0
#define BACKLIGHT_MODE LEDC_LOW_SPEED_MODE
#define BACKLIGHT_RESOLUTION LEDC_TIMER_10_BIT
#define BACKLIGHT_FREQUENCY_HZ 5000

static const char* TAG = "display";

static bool s_open;

static esp_err_t init_backlight(gpio_num_t pin)
{
    const ledc_timer_config_t timer = {
        .speed_mode = BACKLIGHT_MODE,
        .duty_resolution = BACKLIGHT_RESOLUTION,
        .timer_num = BACKLIGHT_TIMER,
        .freq_hz = BACKLIGHT_FREQUENCY_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer), TAG, "LEDC timer");

    const ledc_channel_config_t channel = {
        .gpio_num = pin,
        .speed_mode = BACKLIGHT_MODE,
        .channel = BACKLIGHT_CHANNEL,
        .timer_sel = BACKLIGHT_TIMER,
        .duty = 0,
    };

    return ledc_channel_config(&channel);
}

static esp_err_t init_panel(esp_lcd_panel_handle_t panel)
{
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel), TAG, "reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel), TAG, "init");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel, true), TAG, "invert");

    return esp_lcd_panel_set_gap(panel, X_GAP, Y_GAP);
}

esp_err_t display_open(const display_config_t* config, display_t* out)
{
    ESP_RETURN_ON_FALSE(!s_open, ESP_ERR_INVALID_STATE, TAG, "already open");

    // Dark first: the panel may still show a frame from before the reset.
    ESP_RETURN_ON_ERROR(init_backlight(config->backlight), TAG, "backlight");

    const esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = config->cs,
        .dc_gpio_num = config->dc,
        .spi_mode = 0,
        .pclk_hz = PIXEL_CLOCK_HZ,
        .trans_queue_depth = TRANS_QUEUE_DEPTH,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    esp_lcd_panel_io_handle_t io = NULL;
    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)config->spi_host, &io_config, &io), TAG, "panel IO");

    esp_err_t ret = ESP_OK;
    esp_lcd_panel_handle_t panel = NULL;
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = config->reset,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7789(io, &panel_config, &panel), fail, TAG, "ST7789");
    ESP_GOTO_ON_ERROR(init_panel(panel), fail, TAG, "panel");

    s_open = true;
    *out = (display_t){.io = io, .panel = panel};

    return ESP_OK;

fail:
    if (panel != NULL) {
        esp_lcd_panel_del(panel);
    }
    esp_lcd_panel_io_del(io);

    return ret;
}

esp_err_t display_set_brightness(uint8_t percent)
{
    ESP_RETURN_ON_FALSE(s_open, ESP_ERR_INVALID_STATE, TAG, "not open");
    ESP_RETURN_ON_FALSE(percent <= 100, ESP_ERR_INVALID_ARG, TAG, "percent %u", percent);

    const uint32_t duty = ((1U << BACKLIGHT_RESOLUTION) - 1) * percent / 100;
    ESP_RETURN_ON_ERROR(ledc_set_duty(BACKLIGHT_MODE, BACKLIGHT_CHANNEL, duty), TAG, "duty");

    return ledc_update_duty(BACKLIGHT_MODE, BACKLIGHT_CHANNEL);
}
