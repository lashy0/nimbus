#include <stdio.h>
#include <stdbool.h>

#include "app.h"
#include "bme680_sensor.h"
#include "backlight.h"
#include "buttons.h"
#include "display.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_pm.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "power_manager.h"
#include "sdkconfig.h"
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
#define BME680_HEATER_TEMP_C 300
#define BME680_HEATER_DUR_MS 100
#define BME680_IAQ_BASELINE_MIN_SAMPLES 30
#define BME680_AUTO_RECALIBRATION_INTERVAL_SEC (12 * 60 * 60)
#define UI_ACTIVE_BRIGHTNESS_PCT 60
#define BATTERY_UPDATE_INTERVAL_MS 2000
#define CHARGING_SCREEN_DURATION_MS 3000

static backlight_handle_t bl_handle;
static display_handles_t  disp_hw;
static bool sensor_ready = false;
static bool sensor_calibration_done = false;
static bool sensor_ulp_mode = false;
static lv_timer_t* sensor_timer = NULL;
static bool monitoring_state_prev = false;

static void on_short_press(button_id_t btn_id) { app_on_button_short_press(btn_id); }

static void on_long_press(button_id_t btn_id) { app_on_button_long_press(btn_id); }

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

static void monitoring_state_timer_cb(lv_timer_t* t)
{
    (void)t;
    if (!sensor_ready || sensor_timer == NULL) {
        return;
    }

    bool monitoring = power_manager_is_monitoring();
    if (monitoring == monitoring_state_prev) {
        return;
    }

    monitoring_state_prev = monitoring;
    lv_timer_set_period(sensor_timer, 200);
    lv_timer_ready(sensor_timer);
}

static void sensor_timer_cb(lv_timer_t* t)
{
    static uint32_t read_fail_count = 0;
    static uint32_t battery_read_fail_count = 0;
    static bool charging_screen_shown = false;
    static int64_t charging_screen_hide_deadline_us = 0;
    static power_battery_info_t battery_info = {
        .percent = -1,
        .charging = false,
        .voltage_mv = 0,
        .valid = false,
    };
    static int64_t last_battery_update_us = 0;
    bool charging_transition = false;
    bool charging_now = false;

    app_process_idle();
    bool monitoring = power_manager_is_monitoring();

    int64_t now_us = esp_timer_get_time();
    bool battery_update_due = (last_battery_update_us == 0) ||
                              ((now_us - last_battery_update_us) >= ((int64_t)BATTERY_UPDATE_INTERVAL_MS * 1000LL));
    if (!monitoring && battery_update_due) {
        power_battery_info_t sampled_battery = {0};
        esp_err_t battery_ret = power_manager_read_battery(&sampled_battery);
        if (battery_ret == ESP_OK && sampled_battery.valid) {
            if (battery_info.valid) {
                if (battery_info.charging != sampled_battery.charging) {
                    charging_transition = true;
                    charging_now = sampled_battery.charging;
                }
            } else if (sampled_battery.charging) {
                // First valid sample indicates charging is already active.
                charging_transition = true;
                charging_now = true;
            }

            if (battery_info.charging != sampled_battery.charging) {
                ESP_LOGI(TAG,
                         "Battery state: %s (%u mV, %d%%)",
                         sampled_battery.charging ? "charging" : "battery",
                         (unsigned int)sampled_battery.voltage_mv,
                         sampled_battery.percent);
            }
            battery_info = sampled_battery;
            last_battery_update_us = now_us;
        } else if (battery_ret == ESP_OK && !sampled_battery.valid) {
            if (battery_info.valid) {
                ESP_LOGW(TAG, "Battery state unavailable (%u mV)", (unsigned int)sampled_battery.voltage_mv);
            }
            battery_info = sampled_battery;
            last_battery_update_us = now_us;
        } else {
            battery_read_fail_count++;
            if ((battery_read_fail_count % 20U) == 1U) {
                ESP_LOGW(TAG, "Battery read failed (%s)", esp_err_to_name(battery_ret));
            }
            battery_info.valid = false;
            battery_info.percent = -1;
            battery_info.charging = false;
            last_battery_update_us = now_us;
        }
    }

    bme680_sensor_data_t data = {0};
    bool has_sensor_data = false;
    uint32_t next_period_ms = 0U;
    if (sensor_ready) {
        bool want_ulp_mode = monitoring;
        if (want_ulp_mode != sensor_ulp_mode) {
            esp_err_t mode_ret = bme680_sensor_set_mode(
                want_ulp_mode ? BME680_SENSOR_MODE_ULP : BME680_SENSOR_MODE_LP);
            if (mode_ret == ESP_OK) {
                sensor_ulp_mode = want_ulp_mode;
            } else {
                ESP_LOGW(TAG, "BME680 mode switch failed (%s)", esp_err_to_name(mode_ret));
            }
        }

        esp_err_t ret = bme680_sensor_read(&data);
        if (ret == ESP_OK) {
            has_sensor_data = true;
            next_period_ms = bme680_sensor_get_next_call_delay_ms();
        } else {
            read_fail_count++;
            if ((read_fail_count % 10U) == 1U) {
                ESP_LOGW(TAG, "BME680 read failed (%lu times)", (unsigned long)read_fail_count);
            }
        }
    }

    if (next_period_ms > 0U) {
        lv_timer_set_period(t, next_period_ms);
    }

    if (monitoring) {
        return;
    }

    if (lvgl_port_lock(10))
    {
        if (charging_transition) {
            if (charging_now) {
                if (ui_get_current_screen() != SCREEN_ID_CHARGING) {
                    ui_show_charging();
                }
                charging_screen_shown = true;
                charging_screen_hide_deadline_us = now_us + ((int64_t)CHARGING_SCREEN_DURATION_MS * 1000LL);
            } else if (charging_screen_shown) {
                if (ui_get_current_screen() == SCREEN_ID_CHARGING) {
                    ui_hide_special();
                }
                charging_screen_shown = false;
                charging_screen_hide_deadline_us = 0;
            }
        }

        if (charging_screen_shown &&
            charging_screen_hide_deadline_us > 0 &&
            now_us >= charging_screen_hide_deadline_us) {
            if (ui_get_current_screen() == SCREEN_ID_CHARGING) {
                ui_hide_special();
            }
            charging_screen_shown = false;
            charging_screen_hide_deadline_us = 0;
        }

        if (has_sensor_data) {
            ui_update_calibration_status(data.stabilization_done, data.run_in_done);

            if (data.iaq_valid) {
                if (!sensor_calibration_done) {
                    sensor_calibration_done = true;
                    ESP_LOGI(TAG, "BME680 IAQ calibration completed");
                }

                if (ui_get_current_screen() == SCREEN_ID_CALIBRATION) {
                    loadScreen(SCREEN_ID_IAQ);
                }
            } else {
                if (sensor_calibration_done) {
                    sensor_calibration_done = false;
                    ESP_LOGI(TAG, "BME680 IAQ recalibration started");
                }

                if (ui_get_current_screen() != SCREEN_ID_CALIBRATION) {
                    ui_show_calibration();
                }
            }

            ui_update_iaq((int)data.iaq);
            ui_update_temp((int)data.temperature_c);
            ui_update_hum((int)data.humidity_rh);
        }

        if (battery_info.valid) {
            ui_update_battery(battery_info.percent, battery_info.charging);
        } else {
            ui_update_battery(-1, false);
        }
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
    ESP_ERROR_CHECK(backlight_set_brightness(&bl_handle, UI_ACTIVE_BRIGHTNESS_PCT));

    ESP_LOGI(TAG, "Init power management...");
    init_power_management();

    ESP_LOGI(TAG, "Mount SPIFFS...");
    if (!mount_spiffs()) {
        startup_has_non_critical_error = true;
    }

    ESP_LOGI(TAG, "Init LVGL...");
    init_lvgl();

    ESP_LOGI(TAG, "Init UI...");
    if (lvgl_port_lock(100))
    {
        ui_init();
        lvgl_port_unlock();
    }
    else
    {
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
        .heater_temp_c = BME680_HEATER_TEMP_C,
        .heater_dur_ms = BME680_HEATER_DUR_MS,
        .auto_recalibration_interval_sec = BME680_AUTO_RECALIBRATION_INTERVAL_SEC,
        .baseline_min_samples = BME680_IAQ_BASELINE_MIN_SAMPLES,
        .disable_auto_recalibration = false,
        .disable_state_persistence = false,
    };

    esp_err_t sensor_init_ret = bme680_sensor_init(&bme_cfg);
    if (sensor_init_ret != ESP_OK) {
        bme_cfg.i2c_addr = BME680_I2C_ADDR_HIGH;
        ESP_LOGW(TAG, "BME680 not found at 0x%02X, trying 0x%02X", BME680_I2C_ADDR_LOW, BME680_I2C_ADDR_HIGH);
        sensor_init_ret = bme680_sensor_init(&bme_cfg);
    }

    if (sensor_init_ret == ESP_OK) {
        sensor_ready = true;
        sensor_calibration_done = !bme680_sensor_is_calibrating();
        sensor_ulp_mode = false;
        ESP_LOGI(TAG, "BME680 initialized at I2C address 0x%02X", bme_cfg.i2c_addr);
    } else {
        // startup_has_non_critical_error = true;
        ESP_LOGE(TAG, "BME680 init failed");
        // goto degraded_startup;
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
        ESP_LOGE(TAG, "Buttons init failed");
        goto degraded_startup;
    }

    ESP_LOGI(TAG, "Init UI timers...");
    if (lvgl_port_lock(100))
    {
        sensor_timer = lv_timer_create(sensor_timer_cb, 2000, NULL);
        lv_timer_create(monitoring_state_timer_cb, 500, NULL);
        monitoring_state_prev = power_manager_is_monitoring();
        if (sensor_ready && !sensor_calibration_done) {
            ui_show_calibration();
        }
        ui_finish_startup(startup_has_non_critical_error);
        lvgl_port_unlock();
    }
    else
    {
        startup_has_non_critical_error = true;
        ESP_LOGE(TAG, "Failed to lock LVGL for startup finalization");
        goto degraded_startup;
    }

    ESP_LOGI(TAG, "System started");
    return;

degraded_startup:
    ESP_LOGE(TAG, "Startup degraded: stopping further initialization and runtime");
    if (lvgl_port_lock(100))
    {
        ui_finish_startup(true);
        lvgl_port_unlock();
    }
    else
    {
        ESP_LOGE(TAG, "Failed to lock LVGL to show degraded startup screen");
    }

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
