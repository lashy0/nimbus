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
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
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
#define BME680_IAQ_BASELINE_MIN_SAMPLES 3
#define BME680_AUTO_RECALIBRATION_INTERVAL_SEC (12 * 60 * 60)
#define IAQ_USABLE_ACCURACY 1U

#define UI_ACTIVE_BRIGHTNESS_PCT 60
#define BATTERY_UPDATE_INTERVAL_MS 2000
#define CHARGING_SCREEN_DURATION_MS 3000
#define SENSOR_UI_TIMER_PERIOD_MS 200
#define SENSOR_TASK_STACK_SIZE 8192
#define SENSOR_TASK_PRIORITY 4
#define SENSOR_TASK_LOOP_MS 100
#define SENSOR_DEFAULT_READ_PERIOD_MS 1000
#define STARTUP_LVGL_LOCK_TIMEOUT_MS 300
#define STARTUP_LVGL_LOCK_RETRIES 5
#define STARTUP_LVGL_LOCK_RETRY_DELAY_MS 30

static backlight_handle_t bl_handle;
static display_handles_t disp_hw;
static bool sensor_ready = false;
static bool sensor_calibration_done = false;
static bool sensor_ulp_mode = false;

static lv_timer_t* sensor_ui_timer = NULL;
static TaskHandle_t sensor_task_handle = NULL;
static SemaphoreHandle_t sensor_shared_mutex = NULL;

typedef struct {
    uint32_t read_fail_count;
    uint32_t battery_read_fail_count;
    power_battery_info_t battery_info;
    int64_t last_battery_update_us;
} sensor_worker_state_t;

typedef struct {
    bool charging_screen_shown;
    int64_t charging_screen_hide_deadline_us;
} sensor_ui_runtime_t;

typedef struct {
    bme680_sensor_data_t data;
    bool has_sensor_data;
    uint32_t next_period_ms;
} sensor_sample_result_t;

typedef struct {
    bme680_sensor_data_t latest_sensor_data;
    bool has_sensor_data;
    power_battery_info_t battery_info;
    bool monitoring;
    bool charging_transition_pending;
    bool charging_now;
} sensor_shared_state_t;

typedef enum {
    IAQ_PHASE_UNKNOWN = 0,
    IAQ_PHASE_WARMUP,
    IAQ_PHASE_CAL,
    IAQ_PHASE_READY,
} iaq_phase_t;

static sensor_ui_runtime_t sensor_ui_runtime = {
    .charging_screen_shown = false,
    .charging_screen_hide_deadline_us = 0,
};

static sensor_shared_state_t sensor_shared = {
    .latest_sensor_data = {0},
    .has_sensor_data = false,
    .battery_info =
        {
            .percent = -1,
            .charging = false,
            .voltage_mv = 0,
            .valid = false,
        },
    .monitoring = false,
    .charging_transition_pending = false,
    .charging_now = false,
};

static iaq_phase_t sensor_iaq_phase = IAQ_PHASE_UNKNOWN;
static uint16_t sensor_last_logged_iaq = 0xFFFFU;
static uint8_t sensor_last_logged_accuracy = 0xFFU;
static bool sensor_last_logged_stabilization_done = false;
static bool sensor_last_logged_run_in_done = false;
static bool sensor_last_logged_iaq_valid = false;
static int64_t sensor_next_iaq_log_time_us = 0;

#define IAQ_LOG_PERIOD_US (10LL * 1000000LL)
#define IAQ_LOG_DELTA_TRIGGER 10U

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

static void sensor_step_battery(
    sensor_worker_state_t* state, bool monitoring, int64_t now_us, bool* charging_transition, bool* charging_now)
{
    *charging_transition = false;
    *charging_now = false;

    bool battery_update_due =
        (state->last_battery_update_us == 0) ||
        ((now_us - state->last_battery_update_us) >= ((int64_t)BATTERY_UPDATE_INTERVAL_MS * 1000LL));
    if (monitoring || !battery_update_due) {
        return;
    }

    power_battery_info_t sampled_battery = {0};
    esp_err_t battery_ret = power_manager_read_battery(&sampled_battery);
    if (battery_ret == ESP_OK && sampled_battery.valid) {
        if (state->battery_info.valid) {
            if (state->battery_info.charging != sampled_battery.charging) {
                *charging_transition = true;
                *charging_now = sampled_battery.charging;
            }
        } else if (sampled_battery.charging) {
            *charging_transition = true;
            *charging_now = true;
        }

        if (state->battery_info.charging != sampled_battery.charging) {
            ESP_LOGI(TAG,
                "Battery state: %s (%u mV, %d%%)",
                sampled_battery.charging ? "charging" : "battery",
                (unsigned int)sampled_battery.voltage_mv,
                sampled_battery.percent);
        }

        state->battery_info = sampled_battery;
        state->last_battery_update_us = now_us;
        return;
    }

    if (battery_ret == ESP_OK && !sampled_battery.valid) {
        if (state->battery_info.valid) {
            ESP_LOGW(TAG, "Battery state unavailable (%u mV)", (unsigned int)sampled_battery.voltage_mv);
        }
        state->battery_info = sampled_battery;
        state->last_battery_update_us = now_us;
        return;
    }

    state->battery_read_fail_count++;
    if ((state->battery_read_fail_count % 20U) == 1U) {
        ESP_LOGW(TAG, "Battery read failed (%s)", esp_err_to_name(battery_ret));
    }
    state->battery_info.valid = false;
    state->battery_info.percent = -1;
    state->battery_info.charging = false;
    state->last_battery_update_us = now_us;
}

static sensor_sample_result_t sensor_step_read(sensor_worker_state_t* state, bool monitoring)
{
    sensor_sample_result_t result = {0};
    if (!sensor_ready) {
        return result;
    }

    (void)monitoring;
    bool want_ulp_mode = false;
    if (want_ulp_mode != sensor_ulp_mode) {
        esp_err_t mode_ret = bme680_sensor_set_mode(want_ulp_mode ? BME680_SENSOR_MODE_ULP : BME680_SENSOR_MODE_LP);
        if (mode_ret == ESP_OK) {
            sensor_ulp_mode = want_ulp_mode;
        } else {
            ESP_LOGW(TAG, "BME680 mode switch failed (%s)", esp_err_to_name(mode_ret));
        }
    }

    esp_err_t ret = bme680_sensor_read(&result.data);
    if (ret == ESP_OK) {
        result.has_sensor_data = true;
        result.next_period_ms = bme680_sensor_get_next_call_delay_ms();
        return result;
    }

    state->read_fail_count++;
    if ((state->read_fail_count % 10U) == 1U) {
        ESP_LOGW(TAG, "BME680 read failed (%lu times)", (unsigned long)state->read_fail_count);
    }

    return result;
}

static void sensor_step_charging_overlay(bool charging_transition, bool charging_now, int64_t now_us)
{
    if (charging_transition) {
        if (charging_now) {
            if (ui_get_current_screen() != SCREEN_ID_CHARGING) {
                ui_show_charging();
            }
            sensor_ui_runtime.charging_screen_shown = true;
            sensor_ui_runtime.charging_screen_hide_deadline_us = now_us + ((int64_t)CHARGING_SCREEN_DURATION_MS * 1000LL);
        } else if (sensor_ui_runtime.charging_screen_shown) {
            if (ui_get_current_screen() == SCREEN_ID_CHARGING) {
                ui_hide_special();
            }
            sensor_ui_runtime.charging_screen_shown = false;
            sensor_ui_runtime.charging_screen_hide_deadline_us = 0;
        }
    }

    if (sensor_ui_runtime.charging_screen_shown && sensor_ui_runtime.charging_screen_hide_deadline_us > 0 &&
        now_us >= sensor_ui_runtime.charging_screen_hide_deadline_us) {
        if (ui_get_current_screen() == SCREEN_ID_CHARGING) {
            ui_hide_special();
        }
        sensor_ui_runtime.charging_screen_shown = false;
        sensor_ui_runtime.charging_screen_hide_deadline_us = 0;
    }
}

static iaq_phase_t sensor_detect_iaq_phase(const bme680_sensor_data_t* data)
{
    if (!data || data->iaq_accuracy == 0U) {
        return IAQ_PHASE_WARMUP;
    }
    if (data->stabilization_done && data->run_in_done) {
        return IAQ_PHASE_READY;
    }
    return IAQ_PHASE_CAL;
}

static const char* sensor_iaq_phase_name(iaq_phase_t phase)
{
    switch (phase) {
        case IAQ_PHASE_WARMUP:
            return "WARMUP";
        case IAQ_PHASE_CAL:
            return "CAL";
        case IAQ_PHASE_READY:
            return "READY";
        default:
            return "UNKNOWN";
    }
}

static void sensor_log_iaq_phase_transition(const bme680_sensor_data_t* data)
{
    iaq_phase_t phase = sensor_detect_iaq_phase(data);
    if (phase == sensor_iaq_phase) {
        return;
    }

    sensor_iaq_phase = phase;
    switch (phase) {
        case IAQ_PHASE_WARMUP:
            ESP_LOGI(TAG, "BME680 IAQ state: WARMUP");
            break;
        case IAQ_PHASE_CAL:
            ESP_LOGI(TAG, "BME680 IAQ state: CAL");
            break;
        case IAQ_PHASE_READY:
            ESP_LOGI(TAG, "BME680 IAQ state: READY");
            break;
        default:
            break;
    }
}

static void sensor_log_iaq_snapshot(const bme680_sensor_data_t* data)
{
    if (!data) {
        return;
    }

    int64_t now = esp_timer_get_time();
    iaq_phase_t phase = sensor_detect_iaq_phase(data);

    uint16_t delta = 0U;
    if (sensor_last_logged_iaq != 0xFFFFU) {
        delta = (data->iaq >= sensor_last_logged_iaq) ? (data->iaq - sensor_last_logged_iaq)
                                                      : (sensor_last_logged_iaq - data->iaq);
    }

    bool state_changed = (sensor_last_logged_iaq == 0xFFFFU) || (data->iaq_accuracy != sensor_last_logged_accuracy) ||
                         (data->stabilization_done != sensor_last_logged_stabilization_done) ||
                         (data->run_in_done != sensor_last_logged_run_in_done) ||
                         (data->iaq_valid != sensor_last_logged_iaq_valid);
    bool value_changed = (sensor_last_logged_iaq == 0xFFFFU) || (delta >= IAQ_LOG_DELTA_TRIGGER);
    bool periodic = (sensor_next_iaq_log_time_us == 0) || (now >= sensor_next_iaq_log_time_us);
    if (!state_changed && !value_changed && !periodic) {
        return;
    }

    ESP_LOGI(TAG,
        "IAQ=%u STATIC_IAQ=%u phase=%s acc=%u valid=%s stab=%s run_in=%s mode=%s",
        (unsigned int)data->iaq,
        (unsigned int)data->static_iaq,
        sensor_iaq_phase_name(phase),
        (unsigned int)data->iaq_accuracy,
        data->iaq_valid ? "yes" : "no",
        data->stabilization_done ? "yes" : "no",
        data->run_in_done ? "yes" : "no",
        sensor_ulp_mode ? "ULP" : "LP");

    sensor_last_logged_iaq = data->iaq;
    sensor_last_logged_accuracy = data->iaq_accuracy;
    sensor_last_logged_stabilization_done = data->stabilization_done;
    sensor_last_logged_run_in_done = data->run_in_done;
    sensor_last_logged_iaq_valid = data->iaq_valid;
    sensor_next_iaq_log_time_us = now + IAQ_LOG_PERIOD_US;
}

static void sensor_step_update_sensor_ui(const bme680_sensor_data_t* data, bool has_sensor_data)
{
    if (!has_sensor_data) {
        return;
    }

    ui_update_calibration_status(data->stabilization_done, data->run_in_done);
    ui_update_iaq_quality(data->iaq_accuracy, data->stabilization_done, data->run_in_done);
    sensor_log_iaq_phase_transition(data);

    bool usable_now = (data->iaq_accuracy >= IAQ_USABLE_ACCURACY);
    if (usable_now && !sensor_calibration_done) {
        sensor_calibration_done = true;
        ESP_LOGI(TAG, "BME680 IAQ warmup completed (accuracy=%u)", (unsigned int)data->iaq_accuracy);
    } else if (!usable_now && sensor_calibration_done) {
        sensor_calibration_done = false;
        ESP_LOGI(TAG, "BME680 IAQ warmup started");
    }

    if (usable_now && ui_get_current_screen() == SCREEN_ID_CALIBRATION) {
        loadScreen(SCREEN_ID_IAQ);
    }

    if (usable_now) {
        ui_update_iaq((int)data->static_iaq);
    } else {
        ui_update_iaq(0);
    }
    ui_update_temp((int)data->temperature_c);
    ui_update_hum((int)data->humidity_rh);
}

static void sensor_step_update_battery_ui(const power_battery_info_t* battery_info)
{
    if (battery_info && battery_info->valid) {
        ui_update_battery(battery_info->percent, battery_info->charging);
    } else {
        ui_update_battery(-1, false);
    }
}

static void dispatch_button_events(void)
{
    button_event_msg_t event = {0};
    while (buttons_get_event(&event)) {
        ESP_LOGI(TAG,
            "Button event: %s (%s)",
            (event.button_id == BTN_ID_PREV) ? "PREV" : ((event.button_id == BTN_ID_NEXT) ? "NEXT" : "UNKNOWN"),
            event.is_long_press ? "long" : "short");
        if (event.is_long_press) {
            app_on_button_long_press(event.button_id);
        } else {
            app_on_button_short_press(event.button_id);
        }
    }
}

static void sensor_task(void* arg)
{
    (void)arg;

    sensor_worker_state_t worker_state = {
        .read_fail_count = 0,
        .battery_read_fail_count = 0,
        .battery_info =
            {
                .percent = -1,
                .charging = false,
                .voltage_mv = 0,
                .valid = false,
            },
        .last_battery_update_us = 0,
    };

    bme680_sensor_data_t latest_sensor_data = {0};
    bool has_sensor_data = false;
    int64_t next_sensor_read_us = 0;
    while (1) {
        dispatch_button_events();
        app_process_idle();

        bool monitoring = power_manager_is_monitoring();
        int64_t now = esp_timer_get_time();

        bool charging_transition = false;
        bool charging_now = false;
        sensor_step_battery(&worker_state, monitoring, now, &charging_transition, &charging_now);

        bool should_read = (next_sensor_read_us == 0) || (now >= next_sensor_read_us);
        if (should_read) {
            sensor_sample_result_t sample = sensor_step_read(&worker_state, monitoring);
            if (sample.has_sensor_data) {
                latest_sensor_data = sample.data;
                has_sensor_data = true;
                sensor_log_iaq_snapshot(&latest_sensor_data);
            }

            uint32_t next_period_ms = (sample.next_period_ms > 0U) ? sample.next_period_ms : SENSOR_DEFAULT_READ_PERIOD_MS;
            next_sensor_read_us = now + ((int64_t)next_period_ms * 1000LL);
        }

        if (sensor_shared_mutex && xSemaphoreTake(sensor_shared_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            sensor_shared.latest_sensor_data = latest_sensor_data;
            sensor_shared.has_sensor_data = has_sensor_data;
            sensor_shared.battery_info = worker_state.battery_info;
            sensor_shared.monitoring = monitoring;
            if (charging_transition) {
                sensor_shared.charging_transition_pending = true;
                sensor_shared.charging_now = charging_now;
            }
            xSemaphoreGive(sensor_shared_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_TASK_LOOP_MS));
    }
}

static void sensor_ui_timer_cb(lv_timer_t* t)
{
    (void)t;

    sensor_shared_state_t snapshot = {0};
    snapshot.battery_info.percent = -1;
    snapshot.battery_info.valid = false;

    if (!sensor_shared_mutex) {
        return;
    }

    if (xSemaphoreTake(sensor_shared_mutex, pdMS_TO_TICKS(5)) != pdTRUE) {
        return;
    }

    snapshot = sensor_shared;
    if (sensor_shared.charging_transition_pending) {
        sensor_shared.charging_transition_pending = false;
    }
    xSemaphoreGive(sensor_shared_mutex);

    if (snapshot.monitoring) {
        return;
    }

    if (lvgl_port_lock(10)) {
        int64_t now = esp_timer_get_time();
        sensor_step_charging_overlay(snapshot.charging_transition_pending, snapshot.charging_now, now);
        sensor_step_update_sensor_ui(&snapshot.latest_sensor_data, snapshot.has_sensor_data);
        sensor_step_update_battery_ui(&snapshot.battery_info);
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
    if (lvgl_port_lock(100)) {
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
        .heater_temp_c = BME680_HEATER_TEMP_C,
        .heater_dur_ms = BME680_HEATER_DUR_MS,
        .auto_recalibration_interval_sec = BME680_AUTO_RECALIBRATION_INTERVAL_SEC,
        .baseline_min_samples = BME680_IAQ_BASELINE_MIN_SAMPLES,
        .disable_auto_recalibration = true,
        .disable_state_persistence = false,
        .reset_baseline_on_power_on = true,
    };

    esp_err_t sensor_init_ret = bme680_sensor_init(&bme_cfg);
    if (sensor_init_ret != ESP_OK) {
        bme_cfg.i2c_addr = BME680_I2C_ADDR_HIGH;
        ESP_LOGW(TAG, "BME680 not found at 0x%02X, trying 0x%02X", BME680_I2C_ADDR_LOW, BME680_I2C_ADDR_HIGH);
        sensor_init_ret = bme680_sensor_init(&bme_cfg);
    }

    if (sensor_init_ret == ESP_OK) {
        sensor_ready = true;
        sensor_calibration_done = false;
        sensor_ulp_mode = false;
        sensor_iaq_phase = IAQ_PHASE_UNKNOWN;
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

    sensor_shared_mutex = xSemaphoreCreateMutex();
    if (!sensor_shared_mutex) {
        startup_has_non_critical_error = true;
        ESP_LOGE(TAG, "Failed to create sensor shared mutex");
        goto degraded_startup;
    }

    ESP_LOGI(TAG, "Init UI timers...");
    bool startup_finalized = false;
    for (int attempt = 1; attempt <= STARTUP_LVGL_LOCK_RETRIES; ++attempt) {
        if (lvgl_port_lock(STARTUP_LVGL_LOCK_TIMEOUT_MS)) {
            sensor_ui_timer = lv_timer_create(sensor_ui_timer_cb, SENSOR_UI_TIMER_PERIOD_MS, NULL);
            if (!sensor_ui_timer) {
                startup_has_non_critical_error = true;
                ESP_LOGE(TAG, "Failed to create sensor UI timer");
            }
            ui_finish_startup(startup_has_non_critical_error);
            lvgl_port_unlock();
            startup_finalized = true;
            break;
        }

        ESP_LOGW(TAG,
            "LVGL lock timeout during startup finalization (attempt %d/%d)",
            attempt,
            STARTUP_LVGL_LOCK_RETRIES);
        vTaskDelay(pdMS_TO_TICKS(STARTUP_LVGL_LOCK_RETRY_DELAY_MS));
    }

    if (!startup_finalized) {
        startup_has_non_critical_error = true;
        ESP_LOGE(TAG, "Failed to lock LVGL for startup finalization");
        goto degraded_startup;
    }

    BaseType_t task_ret =
        xTaskCreate(sensor_task, "sensor_task", SENSOR_TASK_STACK_SIZE, NULL, SENSOR_TASK_PRIORITY, &sensor_task_handle);
    if (task_ret != pdPASS) {
        startup_has_non_critical_error = true;
        ESP_LOGE(TAG, "Failed to create sensor task");
        goto degraded_startup;
    }

    if (startup_has_non_critical_error) {
        goto degraded_startup;
    }

    ESP_LOGI(TAG, "System started");
    return;

degraded_startup:
    ESP_LOGE(TAG, "Startup degraded: stopping further initialization and runtime");
    if (lvgl_port_lock(100)) {
        ui_finish_startup(true);
        lvgl_port_unlock();
    } else {
        ESP_LOGE(TAG, "Failed to lock LVGL to show degraded startup screen");
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
