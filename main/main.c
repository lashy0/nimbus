#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "display.h"
#include "backlight.h"
#include "lvgl.h"

#include "lv_demos.h"

static const char *TAG = "main";

static void lv_tick_task(void *arg) {
    lv_tick_inc(2);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing Display...");
    lv_disp_t *disp = display_init_lvgl();
    if (disp == NULL) {
        ESP_LOGE(TAG, "Display init failed");
        return;
    }

    ESP_LOGI(TAG, "Initializing Backlight...");
    backlight_config_t bl_config = {
        .gpio_num = PIN_NUM_BL,
        .leds_mode = LEDC_LOW_SPEED_MODE,
        .leds_channel = LEDC_CHANNEL_0,
        .leds_timer = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 5000
    };
    backlight_handle_t bl_handle;
    ESP_ERROR_CHECK(backlight_init(&bl_config, &bl_handle));
    ESP_ERROR_CHECK(backlight_set_brightness(&bl_handle, 100));

    ESP_LOGI(TAG, "Starting LVGL tick timer...");
    const esp_timer_create_args_t lv_tick_timer_args = {
        .callback = &lv_tick_task,
        .name = "lv_tick"
    };
    esp_timer_handle_t lv_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lv_tick_timer_args, &lv_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lv_tick_timer, 2000));

    ESP_LOGI(TAG, "Starting LVGL Demo...");
    
    lv_demo_music(); 

    ESP_LOGI(TAG, "Entering main loop...");
    while (1) {
        lv_timer_handler();
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}