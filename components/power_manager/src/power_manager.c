#include "power_manager.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "bme680_sensor.h"
#include "ui.h"
#include "esp_lvgl_port.h"

static const char* TAG = "power_mgr";

#define WAKEUP_GPIO GPIO_NUM_0

static power_manager_config_t pm_config;
static bool is_monitoring = false;

void power_manager_init(const power_manager_config_t* config)
{
    pm_config = *config;
}

void power_manager_check_wakeup_reason(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    switch (cause)
    {
        case ESP_SLEEP_WAKEUP_EXT0:
            ESP_LOGI(TAG, "Wakeup from button press");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            ESP_LOGI(TAG, "Wakeup from EXT1");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Wakeup from timer");
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            ESP_LOGI(TAG, "Normal startup");
            break;
    }
}

void power_manager_shutdown(void)
{
    ESP_LOGI(TAG, "Shutting down...");

    if (lvgl_port_lock(100)) {
        ui_show_no_charging();
        lvgl_port_unlock();
    }

    vTaskDelay(pdMS_TO_TICKS(500));

    if (pm_config.bl_handle) {
        backlight_set_brightness(pm_config.bl_handle, 0);
    }

    if (pm_config.panel_handle) {
        esp_lcd_panel_disp_on_off(pm_config.panel_handle, false);
    }

    esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 0);
    bme680_sensor_deinit();

    ESP_LOGI(TAG, "Entering deep sleep. Press button to wake up.");

    esp_deep_sleep_start();
}

void power_manager_enter_monitoring(void)
{
    ESP_LOGI(TAG, "Entering monitoring mode...");
    is_monitoring = true;

    if (pm_config.bl_handle) {
        backlight_set_brightness(pm_config.bl_handle, 0);
    }

    if (pm_config.panel_handle) {
        esp_lcd_panel_disp_on_off(pm_config.panel_handle, false);
    }
}

void power_manager_exit_monitoring(void)
{
    ESP_LOGI(TAG, "Exiting monitoring mode...");
    is_monitoring = false;

    if (pm_config.panel_handle) {
        esp_lcd_panel_disp_on_off(pm_config.panel_handle, true);
    }

    if (pm_config.bl_handle) {
        backlight_set_brightness(pm_config.bl_handle, 100);
    }
}

bool power_manager_is_monitoring(void)
{
    return is_monitoring;
}
