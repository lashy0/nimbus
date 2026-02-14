#include <stdio.h>
#include <stdbool.h>

#include "app.h"
#include "backlight.h"
#include "buttons.h"
#include "display.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "ui.h"

static const char* TAG = "main";

#define BUTTON_PREV_GPIO GPIO_NUM_0
#define BUTTON_NEXT_GPIO GPIO_NUM_35

static backlight_handle_t bl_handle;
static display_handles_t  disp_hw;

static void on_short_press(button_id_t btn_id) { app_on_button_short_press(btn_id); }

static void on_long_press(button_id_t btn_id) { app_on_button_long_press(btn_id); }

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

static bool mount_spiffs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
        return false;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "SPIFFS: total=%d, used=%d", total, used);
    }

    return true;
}

static void init_lvgl(void)
{
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 4096,
        .task_affinity = -1,
        .timer_period_ms = 2,
    };
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

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
}

void app_main(void)
{
    bool startup_has_non_critical_error = false;

    ESP_LOGI(TAG, "Init Display...");
    disp_hw = display_init();

    ESP_LOGI(TAG, "Init Backlight...");
    backlight_config_t bl_config = {
        .gpio_num = PIN_NUM_BL,
        .leds_mode = LEDC_LOW_SPEED_MODE,
        .leds_channel = LEDC_CHANNEL_0,
        .leds_timer = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 5000,
    };
    ESP_ERROR_CHECK(backlight_init(&bl_config, &bl_handle));
    ESP_ERROR_CHECK(backlight_set_brightness(&bl_handle, 100));

    ESP_LOGI(TAG, "Mount SPIFFS...");
    if (!mount_spiffs()) {
        startup_has_non_critical_error = true;
    }

    ESP_LOGI(TAG, "Init LVGL...");
    init_lvgl();

    ESP_LOGI(TAG, "Init UI...");
    if (lvgl_port_lock(0))
    {
        ui_init();
        lv_timer_create(ui_tick_timer_cb, 5, NULL);
        lvgl_port_unlock();
    }

    ESP_LOGI(TAG, "Init app...");
    app_config_t app_cfg = {
        .display = &disp_hw,
        .backlight = &bl_handle,
    };
    app_init(&app_cfg);

    ESP_LOGI(TAG, "Init buttons...");
    buttons_config_t btn_cfg = {
        .prev_gpio = BUTTON_PREV_GPIO,
        .next_gpio = BUTTON_NEXT_GPIO,
        .long_press_time_ms = 1500,
        .short_press_time_ms = 50,
        .on_short_press = on_short_press,
        .on_long_press = on_long_press,
    };
    if (!buttons_init(&btn_cfg)) {
        startup_has_non_critical_error = true;
    }

    ESP_LOGI(TAG, "Init UI timers...");
    if (lvgl_port_lock(0))
    {
        lv_timer_create(sensor_timer_cb, 2000, NULL);
        ui_finish_startup(startup_has_non_critical_error);
        lvgl_port_unlock();
    }

    ESP_LOGI(TAG, "System started");
}
