#include "sensing.h"

#include <stdatomic.h>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "bme680.h"
#include "sensing_bsec.h"
#include "sensing_state.h"

#define TASK_STACK_BYTES 4096
#define TASK_PRIORITY 5
#define RETRY_DELAY_NS (3000LL * 1000 * 1000)
#define STATE_SAVE_PERIOD_NS (60LL * 60 * 1000 * 1000 * 1000)
#define IAQ_ACCURACY_HIGH 3
#define STOP_TIMEOUT_MS 2000

static const char* TAG = "sensing";

static bme680_handle_t s_sensor;
static TaskHandle_t s_task;
static atomic_bool s_stop_requested;
static StaticSemaphore_t s_stopped_buffer;
static SemaphoreHandle_t s_stopped; // Given by the task once it has stopped.

// Used by the sensing task only.
static int64_t s_state_saved_ns;
static bool s_accuracy_high_seen;

static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static sensing_snapshot_t s_snapshot; // Guarded by s_lock.

static void publish(esp_err_t err, const sensing_bsec_outputs_t* outputs)
{
    taskENTER_CRITICAL(&s_lock);

    if (err != ESP_OK) {
        s_snapshot.status = SENSING_STATUS_ERROR;
    } else if (outputs->count > 0) {
        s_snapshot = (sensing_snapshot_t){
            .status = SENSING_STATUS_OK,
            .temperature_c = outputs->temperature_c,
            .humidity_percent = outputs->humidity_percent,
            .pressure_hpa = outputs->pressure_hpa,
            .iaq = outputs->iaq,
            .iaq_accuracy = outputs->iaq_accuracy,
        };
    }

    taskEXIT_CRITICAL(&s_lock);
}

// Saves when accuracy first reaches high, then once per period.
// After a failed save, the next try is a period later.
static void save_state_if_due(const sensing_bsec_outputs_t* outputs, int64_t now_ns)
{
    const bool first_high = !s_accuracy_high_seen && outputs->iaq_accuracy == IAQ_ACCURACY_HIGH;
    if (!first_high && now_ns - s_state_saved_ns < STATE_SAVE_PERIOD_NS) {
        return;
    }

    s_accuracy_high_seen |= first_high;
    s_state_saved_ns = now_ns;

    const esp_err_t err = sensing_state_save();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "BSEC state saved (accuracy %u)", outputs->iaq_accuracy);
    } else {
        ESP_LOGW(TAG, "BSEC state not saved: %s", esp_err_to_name(err));
    }
}

static void restore_state(void)
{
    const esp_err_t err = sensing_state_load();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "BSEC state restored");
    } else if (err == ESP_ERR_NOT_FOUND) {
        ESP_LOGI(TAG, "no saved BSEC state, calibrating from scratch");
    } else {
        ESP_LOGW(TAG, "BSEC state not restored, calibrating from scratch: %s", esp_err_to_name(err));
    }
}

// Waits until time_ns, never less. Returns false if a stop is requested first.
static bool wait_until(int64_t time_ns)
{
    for (;;) {
        if (atomic_load(&s_stop_requested)) {
            return false;
        }

        const int64_t wait_us = (time_ns + 999) / 1000 - esp_timer_get_time();
        if (wait_us <= 0) {
            return true;
        }

        // One tick more: a wait of n ticks may end right after the next tick boundary.
        ulTaskNotifyTake(pdTRUE, (TickType_t)((wait_us * configTICK_RATE_HZ + 999999) / 1000000) + 1);
    }
}

static void sensing_task(void* arg)
{
    int64_t next_call_ns = 0;

    for (;;) {
        sensing_bsec_outputs_t outputs;
        const esp_err_t err = sensing_bsec_step(s_sensor, &outputs, &next_call_ns);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "step: %s", esp_err_to_name(err));
        }

        publish(err, &outputs);

        const int64_t now_ns = esp_timer_get_time() * 1000;
        if (err == ESP_OK && outputs.count > 0) {
            save_state_if_due(&outputs, now_ns);
        }

        // A failed step may leave next_call in the past.
        if (next_call_ns <= now_ns) {
            next_call_ns = now_ns + RETRY_DELAY_NS;
        }

        if (!wait_until(next_call_ns)) {
            break;
        }
    }

    const esp_err_t err = sensing_state_save();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "BSEC state not saved: %s", esp_err_to_name(err));
    }

    xSemaphoreGive(s_stopped);
    vTaskDelete(NULL);
}

esp_err_t sensing_start(i2c_master_bus_handle_t bus, BaseType_t core)
{
    ESP_RETURN_ON_FALSE(
        core == tskNO_AFFINITY || (core >= 0 && core < portNUM_PROCESSORS), ESP_ERR_INVALID_ARG, TAG, "core");
    ESP_RETURN_ON_FALSE(s_task == NULL, ESP_ERR_INVALID_STATE, TAG, "already started");
    ESP_RETURN_ON_ERROR(bme680_open(bus, &s_sensor), TAG, "open BME680");
    s_stopped = xSemaphoreCreateBinaryStatic(&s_stopped_buffer);

    esp_err_t ret = ESP_OK;
    ESP_GOTO_ON_ERROR(sensing_bsec_init(), fail, TAG, "BSEC");
    restore_state();
    ESP_GOTO_ON_ERROR(sensing_bsec_subscribe(), fail, TAG, "BSEC subscription");
    ESP_GOTO_ON_FALSE(
        xTaskCreatePinnedToCore(sensing_task, "sensing", TASK_STACK_BYTES, NULL, TASK_PRIORITY, &s_task, core) ==
            pdPASS,
        ESP_ERR_NO_MEM,
        fail,
        TAG,
        "task");

    return ESP_OK;

fail:
    bme680_close(s_sensor);
    s_sensor = NULL;

    return ret;
}

void sensing_get_snapshot(sensing_snapshot_t* out)
{
    taskENTER_CRITICAL(&s_lock);
    *out = s_snapshot;
    taskEXIT_CRITICAL(&s_lock);
}

esp_err_t sensing_stop(void)
{
    ESP_RETURN_ON_FALSE(s_task != NULL, ESP_ERR_INVALID_STATE, TAG, "not started");
    ESP_RETURN_ON_FALSE(!atomic_exchange(&s_stop_requested, true), ESP_ERR_INVALID_STATE, TAG, "already stopped");

    xTaskNotifyGive(s_task);
    ESP_RETURN_ON_FALSE(
        xSemaphoreTake(s_stopped, pdMS_TO_TICKS(STOP_TIMEOUT_MS)) == pdTRUE, ESP_ERR_TIMEOUT, TAG, "no confirmation");

    return ESP_OK;
}
