#include <stdio.h>

#include "backlight.h"
#include "button_gpio.h"
#include "display.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iot_button.h"
#include "lvgl.h"
#include "ui.h"

static const char* TAG = "main";

#define BUTTON_PREV_GPIO GPIO_NUM_0
#define BUTTON_NEXT_GPIO GPIO_NUM_35

static void ui_tick_timer_cb(lv_timer_t* t) { ui_tick(); }

// Test ui
static void sensor_timer_cb(lv_timer_t* t)
{
    static int counter = 0;
    counter++;
    
    if (lvgl_port_lock(10))
    {
        ui_update_iaq((counter * 7) % 500);
        ui_update_temp(-10 + (counter % 50));
        ui_update_hum(counter % 100);
        ui_update_battery(100 - (counter % 100), (counter / 10) % 2);
        lvgl_port_unlock();
    }
}

static void button_press_cb(void* arg, void* data)
{
    int btn_id = (int)data;

    if (lvgl_port_lock(-1))
    {
        if (btn_id == 0)
        {
            ESP_LOGI(TAG, "Button PREV pressed");
            ui_switch_prev();
        }
        else
        {
            ESP_LOGI(TAG, "Button NEXT pressed");
            ui_switch_next();
        }

        lvgl_port_unlock();
    }
}

void mount_spiffs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs", .partition_label = "storage", .max_files = 5, .format_if_mount_failed = true};

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE("SPIFFS", "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE("SPIFFS", "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE("SPIFFS", "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE("SPIFFS", "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI("SPIFFS", "Partition size: total: %d, used: %d", total, used);
    }
}

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

    mount_spiffs();

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
        .buffer_size = WIDTH * HEIGHT / 10,
        .double_buffer = true,
        .hres = WIDTH,
        .vres = HEIGHT,
        .monochrome = false,
        .rotation =
            {
                .swap_xy = false,
                .mirror_x = false,
                .mirror_y = false,
            },
        .flags =
            {
                .buff_dma = true,
                .buff_spiram = false,
                .full_refresh = false,
            },
    };

    lvgl_port_add_disp(&disp_cfg);

    // Buttons
    const button_config_t btn_cfg = {
        .long_press_time = 1000,
        .short_press_time = 50,
    };

    button_gpio_config_t gpio_cfg_prev = {
        .gpio_num = BUTTON_PREV_GPIO,
        .active_level = 0,
    };

    button_gpio_config_t gpio_cfg_next = {
        .gpio_num = BUTTON_NEXT_GPIO,
        .active_level = 0,
        .disable_pull = true,
    };

    button_handle_t btn_prev_handle = NULL;
    button_handle_t btn_next_handle = NULL;

    esp_err_t err_btn;
    ESP_LOGI(TAG, "Creating PREV button on GPIO %d...", BUTTON_PREV_GPIO);
    err_btn = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg_prev, &btn_prev_handle);
    if (err_btn != ESP_OK)
    {
        ESP_LOGE(TAG, "Button PREV create failed with error: %s", esp_err_to_name(err_btn));
    }
    else
    {
        ESP_LOGI(TAG, "Button PREV created successfully");
        iot_button_register_cb(btn_prev_handle, BUTTON_PRESS_DOWN, NULL, button_press_cb, (void*)0);
    }

    ESP_LOGI(TAG, "Creating NEXT button on GPIO %d...", BUTTON_NEXT_GPIO);
    err_btn = iot_button_new_gpio_device(&btn_cfg, &gpio_cfg_next, &btn_next_handle);
    if (err_btn != ESP_OK)
    {
        ESP_LOGE(TAG, "Button NEXT create failed with error: %s", esp_err_to_name(err_btn));
    }
    else
    {
        ESP_LOGI(TAG, "Button NEXT created successfully");
        iot_button_register_cb(btn_next_handle, BUTTON_PRESS_DOWN, NULL, button_press_cb, (void*)1);
    }

    if (lvgl_port_lock(0))
    {
        ui_init();

        lv_timer_create(sensor_timer_cb, 2000, NULL);

        lv_timer_create(ui_tick_timer_cb, 5, NULL);
        lvgl_port_unlock();
    }

    ESP_LOGI(TAG, "System started");
}