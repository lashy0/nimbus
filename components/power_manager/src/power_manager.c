#include "power_manager_internal.h"

#include "esp_log.h"
#include "esp_sleep.h"

static const char* TAG = "power_mgr";

power_manager_config_t s_pm_config;
bool s_is_monitoring = false;

void power_manager_init(const power_manager_config_t* config)
{
    s_pm_config = *config;
    pm_battery_init();
    pm_brightness_init();
    pm_brightness_apply_current();
}

void power_manager_check_wakeup_reason(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    switch (cause) {
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

bool power_manager_is_monitoring(void)
{
    return s_is_monitoring;
}
