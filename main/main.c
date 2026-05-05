#include <stdbool.h>
#include <stdio.h>

#include "app.h"
#include "backlight.h"
#include "bme680_sensor.h"
#include "buttons.h"
#include "display.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_pm.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "freertos/task.h"
#include "fonts.h"
#include "lvgl.h"
#include "power_manager.h"
#include "sdkconfig.h"
#include "sensor_runtime.h"
#include "ui.h"

static const char* TAG = "main";

#define BUTTON_PREV_GPIO GPIO_NUM_0
#define BUTTON_NEXT_GPIO GPIO_NUM_35

#define BME680_I2C_PORT I2C_NUM_0
#define BME680_I2C_SDA_GPIO GPIO_NUM_21
#define BME680_I2C_SCL_GPIO GPIO_NUM_22
#define BME680_I2C_SPEED_HZ 100000
#define BME680_I2C_ADDR_LOW 0x76
#define BME680_I2C_ADDR_HIGH 0x77

#define STARTUP_LVGL_LOCK_TIMEOUT_MS 300
#define STARTUP_LVGL_LOCK_RETRIES 5
#define STARTUP_LVGL_LOCK_RETRY_DELAY_MS 30

static backlight_handle_t bl_handle;
static display_handles_t disp_hw;

static void init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS requires erase, retrying");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "NVS init failed: %s", esp_err_to_name(ret));
    }
}

static void init_power_management(void)
{
#if CONFIG_PM_ENABLE
    const esp_pm_config_t pm_config = {
        .max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
        .min_freq_mhz = 40,
        .light_sleep_enable = true,
    };
    esp_err_t ret = esp_pm_configure(&pm_config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Power management enabled (DFS + light sleep)");
    } else {
        ESP_LOGW(TAG, "Failed to configure power management: %s", esp_err_to_name(ret));
    }
#else
    ESP_LOGI(TAG, "Power management disabled in sdkconfig");
#endif
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
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
        return false;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret == ESP_OK) {
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

static bool startup_lock_lvgl_with_retry(const char* operation)
{
    for (int attempt = 1; attempt <= STARTUP_LVGL_LOCK_RETRIES; ++attempt) {
        if (lvgl_port_lock(STARTUP_LVGL_LOCK_TIMEOUT_MS)) {
            return true;
        }

        ESP_LOGW(TAG, "LVGL lock timeout during %s (attempt %d/%d)", operation, attempt, STARTUP_LVGL_LOCK_RETRIES);
        vTaskDelay(pdMS_TO_TICKS(STARTUP_LVGL_LOCK_RETRY_DELAY_MS));
    }

    return false;
}

void app_main(void)
{
    bool startup_has_non_critical_error = false;

    ESP_LOGI(TAG, "Init NVS...");
    init_nvs();

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
    // Keep panel dark during startup to avoid visible white frame before UI is rendered.
    ESP_ERROR_CHECK(backlight_set_brightness(&bl_handle, 0));

    ESP_LOGI(TAG, "Init power management...");
    init_power_management();

    ESP_LOGI(TAG, "Mount SPIFFS...");
    if (!mount_spiffs()) {
        startup_has_non_critical_error = true;
    }

    ESP_LOGI(TAG, "Init LVGL...");
    init_lvgl();

    ESP_LOGI(TAG, "Load UI fonts...");
    if (startup_lock_lvgl_with_retry("font loading")) {
        if (!ui_fonts_init()) {
            startup_has_non_critical_error = true;
            ESP_LOGW(TAG, "Some runtime fonts failed to load");
        }
        lvgl_port_unlock();
    } else {
        startup_has_non_critical_error = true;
        ESP_LOGE(TAG, "Failed to lock LVGL for font loading");
    }

    ESP_LOGI(TAG, "Init UI...");
    if (startup_lock_lvgl_with_retry("initial UI setup")) {
        ui_init();
        lvgl_port_unlock();
    } else {
        startup_has_non_critical_error = true;
        ESP_LOGE(TAG, "Failed to lock LVGL for initial UI setup");
    }

    if (startup_has_non_critical_error) {
        goto degraded_startup;
    }

    ESP_LOGI(TAG, "Init BME680...");
    bme680_sensor_config_t bme_cfg = {
        .i2c_port = BME680_I2C_PORT,
        .sda_io_num = BME680_I2C_SDA_GPIO,
        .scl_io_num = BME680_I2C_SCL_GPIO,
        .i2c_clk_speed_hz = BME680_I2C_SPEED_HZ,
        .i2c_addr = BME680_I2C_ADDR_LOW,
        .disable_state_persistence = false,
        .reset_baseline_on_power_on = false,
    };

    if (!bme680_sensor_probe(&bme_cfg)) {
        bme_cfg.i2c_addr = BME680_I2C_ADDR_HIGH;
    }

    esp_err_t sensor_init_ret = bme680_sensor_init(&bme_cfg);
    if (sensor_init_ret == ESP_OK) {
        ESP_LOGI(TAG, "BME680 initialized at I2C address 0x%02X", bme_cfg.i2c_addr);
    } else {
        ESP_LOGE(TAG, "BME680 init failed");
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
        .on_short_press = NULL,
        .on_long_press = NULL,
    };
    if (!buttons_init(&btn_cfg)) {
        startup_has_non_critical_error = true;
        ESP_LOGE(TAG, "Buttons init failed");
        goto degraded_startup;
    }

    ESP_LOGI(TAG, "Finalize startup UI...");
    if (startup_lock_lvgl_with_retry("startup finalization")) {
        ui_finish_startup(startup_has_non_critical_error);
        lvgl_port_unlock();
    } else {
        startup_has_non_critical_error = true;
        ESP_LOGE(TAG, "Failed to lock LVGL for startup finalization");
        goto degraded_startup;
    }

    ESP_LOGI(TAG, "Start sensor runtime...");
    if (sensor_runtime_start() != ESP_OK) {
        startup_has_non_critical_error = true;
        goto degraded_startup;
    }

    if (startup_has_non_critical_error) {
        goto degraded_startup;
    }

    ESP_LOGI(TAG, "System started");
    return;

degraded_startup:
    ESP_LOGE(TAG, "Startup degraded: rebooting in 10 seconds");
    if (startup_lock_lvgl_with_retry("degraded startup UI")) {
        ui_finish_startup(true);
        lvgl_port_unlock();
    } else {
        ESP_LOGE(TAG, "Failed to lock LVGL to show degraded startup screen");
    }

    vTaskDelay(pdMS_TO_TICKS(10000));
    ESP_LOGE(TAG, "Rebooting after degraded startup");
    esp_restart();
}
