#include "power.h"

#include "esp_check.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/task.h"

#include "power_battery.h"
#include "power_estimate.h"

#define TASK_STACK_BYTES 3072
#define TASK_PRIORITY 3
#define PERIOD_MS 500

static const char* TAG = "power";

static TaskHandle_t s_task;
static power_estimate_t s_estimate; // Used by the power task only.

static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static power_status_t s_status; // Guarded by s_lock.

static void measure(void)
{
    uint32_t battery_mv = 0;
    const esp_err_t err = power_battery_read(&battery_mv);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "read: %s", esp_err_to_name(err));
    }

    const bool was_critical = s_estimate.status.critical;
    power_estimate_update(&s_estimate, err == ESP_OK, battery_mv);
    if (s_estimate.status.critical && !was_critical) {
        ESP_LOGW(TAG, "battery critical: %u mV", s_estimate.status.battery_mv);
    }

    taskENTER_CRITICAL(&s_lock);
    s_status = s_estimate.status;
    taskEXIT_CRITICAL(&s_lock);
}

static void power_task(void* arg)
{
    TickType_t wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(PERIOD_MS));
        measure();
    }
}

esp_err_t power_start(const power_config_t* config, BaseType_t core)
{
    ESP_RETURN_ON_FALSE(
        core == tskNO_AFFINITY || (core >= 0 && core < portNUM_PROCESSORS), ESP_ERR_INVALID_ARG, TAG, "core");
    ESP_RETURN_ON_FALSE(s_task == NULL, ESP_ERR_INVALID_STATE, TAG, "already started");
    ESP_RETURN_ON_ERROR(power_battery_init(config->battery, config->adc_enable), TAG, "battery");

    power_estimate_init(&s_estimate);
    measure();
    ESP_RETURN_ON_FALSE(
        xTaskCreatePinnedToCore(power_task, "power", TASK_STACK_BYTES, NULL, TASK_PRIORITY, &s_task, core) == pdPASS,
        ESP_ERR_NO_MEM,
        TAG,
        "task");

    return ESP_OK;
}

void power_get_status(power_status_t* out)
{
    taskENTER_CRITICAL(&s_lock);
    *out = s_status;
    taskEXIT_CRITICAL(&s_lock);
}

bool power_startup_allowed(void)
{
    power_status_t status;
    power_get_status(&status);

    const bool after_brownout = esp_reset_reason() == ESP_RST_BROWNOUT;
    const bool allowed = power_estimate_startup_allowed(&status, after_brownout);
    if (!allowed) {
        ESP_LOGW(TAG, "too low to start: %u mV%s", status.battery_mv, after_brownout ? " after a brownout" : "");
    }

    return allowed;
}
