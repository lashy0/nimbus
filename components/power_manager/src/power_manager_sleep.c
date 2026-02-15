#include "power_manager_internal.h"

#include "bme680_sensor.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui.h"

static const char* TAG = "power_mgr";

#define WAKEUP_GPIO GPIO_NUM_0

static void display_power_down(void)
{
    if (s_pm_config.bl_handle) {
        backlight_set_brightness(s_pm_config.bl_handle, 0);
    }

    if (s_pm_config.panel_handle) {
        esp_lcd_panel_disp_on_off(s_pm_config.panel_handle, false);
    }
}

static void display_power_up(void)
{
    if (s_pm_config.panel_handle) {
        esp_lcd_panel_disp_on_off(s_pm_config.panel_handle, true);
    }

    if (s_pm_config.bl_handle) {
        backlight_set_brightness(s_pm_config.bl_handle, power_manager_get_active_brightness());
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
    display_power_down();

    // Ensure deep sleep wakeup sources are deterministic: button only.
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_sleep_enable_ext0_wakeup(WAKEUP_GPIO, 0);
    bme680_sensor_deinit();

    ESP_LOGI(TAG, "Entering deep sleep. Press button to wake up.");
    esp_deep_sleep_start();
}

void power_manager_enter_monitoring(void)
{
    ESP_LOGI(TAG, "Entering monitoring mode...");
    s_is_monitoring = true;
    display_power_down();
}

void power_manager_exit_monitoring(void)
{
    ESP_LOGI(TAG, "Exiting monitoring mode...");
    s_is_monitoring = false;
    display_power_up();
}
