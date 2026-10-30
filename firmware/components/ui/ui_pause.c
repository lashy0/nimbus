#include "ui_pause.h"

#include <stdatomic.h>
#include <stdbool.h>

#include "esp_check.h"
#include "esp_lvgl_port.h"
#include "freertos/task.h"

#include "buttons.h"
#include "power.h"

#define TASK_STACK_BYTES 3072
#define TASK_PRIORITY 2
#define CHECK_PERIOD_MS 500

static const char* TAG = "ui_pause";

static TaskHandle_t s_task;
static atomic_bool s_disabled;

static void wait_for_resume(void)
{
    power_status_t status;
    power_get_status(&status);
    const power_source_t source = status.source;

    while (!atomic_load(&s_disabled) && !buttons_wait(pdMS_TO_TICKS(CHECK_PERIOD_MS))) {
        power_get_status(&status);
        if (status.source != source) {
            return;
        }
    }
}

// The LVGL task blocks on the held lock; lvgl_port_stop() alone leaves it polling.
static void pause_task(void* arg)
{
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        lvgl_port_lock(0);
        if (!atomic_load(&s_disabled)) {
            lvgl_port_stop();
            wait_for_resume();
            lvgl_port_resume();
        }
        lvgl_port_unlock();
    }
}

esp_err_t ui_pause_start(BaseType_t core)
{
    ESP_RETURN_ON_FALSE(
        xTaskCreatePinnedToCore(pause_task, "ui_pause", TASK_STACK_BYTES, NULL, TASK_PRIORITY, &s_task, core) == pdPASS,
        ESP_ERR_NO_MEM,
        TAG,
        "task");

    return ESP_OK;
}

void ui_pause_request(void)
{
    xTaskNotifyGive(s_task);
}

void ui_pause_disable(void)
{
    atomic_store(&s_disabled, true);
}
