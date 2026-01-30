#include <stdio.h>

#include "backlight.h"
#include "display.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lv_demos.h"
#include "lvgl.h"

static const char* TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing Display...");
    display_handles_t disp_hw = display_init();

    ESP_LOGI(TAG, "Initializing Backlight...");
    backlight_config_t bl_config = {
        .gpio_num = PIN_NUM_BL,
        .leds_mode = LEDC_LOW_SPEED_MODE,
        .leds_channel = LEDC_CHANNEL_0,
        .leds_timer = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 5000,
    };
    backlight_handle_t bl_handle;
    ESP_ERROR_CHECK(backlight_init(&bl_config, &bl_handle));
    ESP_ERROR_CHECK(backlight_set_brightness(&bl_handle, 100));

    ESP_LOGI(TAG, "Init LVGL Port...");
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 4096,
        .task_affinity = -1,
        .timer_period_ms = 2,
    };
    esp_err_t err = lvgl_port_init(&lvgl_cfg);
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "Add Display to LVGL...");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = disp_hw.io_handle,
        .panel_handle = disp_hw.panel_handle,
        .buffer_size = HEIGHT * 20,
        .double_buffer = true,
        .hres = HEIGHT,
        .vres = WIDTH,
        .monochrome = false,
        .rotation =
            {
                .swap_xy = true,
                .mirror_x = false,
                .mirror_y = true,
            },
        .flags =
            {
                .buff_dma = true,
                .buff_spiram = false,
                .full_refresh = false,
            },
    };

    lvgl_port_add_disp(&disp_cfg);

    if (lvgl_port_lock(0))
    {
        lv_demo_music();

        lvgl_port_unlock();
    }

    ESP_LOGI(TAG, "System started");
}