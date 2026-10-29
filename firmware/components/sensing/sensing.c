#include "sensing.h"

#include <unistd.h>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bme680.h"
#include "sensing_bsec.h"

#define TASK_STACK_BYTES 4096
#define TASK_PRIORITY 5
#define RETRY_DELAY_NS (3000LL * 1000 * 1000)

static const char* TAG = "sensing";

static bme680_handle_t s_sensor;
static TaskHandle_t s_task;

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

// Rounded up, so the task does not wake before next_call.
static void wait_until(int64_t time_ns)
{
    const int64_t wait_us = (time_ns + 999) / 1000 - esp_timer_get_time();
    if (wait_us > 0) {
        usleep(wait_us);
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

        // A failed step may leave next_call in the past.
        const int64_t now_ns = esp_timer_get_time() * 1000;
        if (next_call_ns <= now_ns) {
            next_call_ns = now_ns + RETRY_DELAY_NS;
        }

        wait_until(next_call_ns);
    }
}

esp_err_t sensing_start(i2c_master_bus_handle_t bus)
{
    ESP_RETURN_ON_FALSE(s_task == NULL, ESP_ERR_INVALID_STATE, TAG, "already started");
    ESP_RETURN_ON_ERROR(bme680_open(bus, &s_sensor), TAG, "open BME680");

    esp_err_t ret = ESP_OK;
    ESP_GOTO_ON_ERROR(sensing_bsec_init(), fail, TAG, "BSEC");
    ESP_GOTO_ON_FALSE(
        xTaskCreate(sensing_task, "sensing", TASK_STACK_BYTES, NULL, TASK_PRIORITY, &s_task) == pdPASS,
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
