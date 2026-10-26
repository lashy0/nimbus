#include "sensing.h"

#include <stdatomic.h>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sensing_internal.h"

#define TASK_STACK_BYTES 6144
#define TASK_PRIORITY 4
#define WDT_SLICE_MS 5000 // Long waits are split so the 10 s task watchdog keeps being fed.
#define LOG_EVERY_OUTPUTS 20

static const char* TAG = "sensing";

static sensing_config_t s_config;
static TaskHandle_t s_task;
static SemaphoreHandle_t s_lock;
static sensing_snapshot_t s_published;
static int64_t s_published_deadline_ms; // Protected by s_lock with s_published.

static sensing_snapshot_t s_snapshot; // Last complete result, owned by the task.
static sensing_outputs_t s_outputs;   // Partial BSEC outputs, never exposed to consumers.
static uint32_t s_measurement_count;
static uint8_t s_received;
static int64_t s_batch_started_ms;
static int64_t s_result_deadline_ms;
static sensing_sensor_t s_sensor;
static sensing_bsec_t s_bsec;
static sensing_checkpoint_t s_checkpoint;

static atomic_bool s_stop_requested;
static SemaphoreHandle_t s_stopped; // Given once the task has saved and parked.

static const char* phase_name(sensing_phase_t phase)
{
    static const char* const NAMES[] = {"initializing", "error", "warmup", "calibrating", "ready"};

    return NAMES[phase];
}

static void publish(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_published = s_snapshot;
    s_published_deadline_ms = s_result_deadline_ms;
    xSemaphoreGive(s_lock);

    if (s_config.notify != NULL) {
        s_config.notify(s_config.ctx);
    }
}

static void set_phase(sensing_phase_t phase)
{
    if (phase != s_snapshot.phase) {
        ESP_LOGI(TAG, "phase %s -> %s", phase_name(s_snapshot.phase), phase_name(phase));
        s_snapshot.phase = phase;
    }
}

// Partial values from before a failure must not enter a recovered measurement.
static void enter_error(void)
{
    s_outputs = (sensing_outputs_t){0};
    s_received = 0;
    set_phase(SENSING_PHASE_ERROR);
    publish();
}

// Returns once esp_timer reaches deadline_us; sensing_stop() wakes the task earlier
// through its notification. Ticks are rounded up and one is added: a wait of n
// ticks can end right after the next tick boundary, which would call BSEC before
// next_call and cost an extra wake-up.
static void wait_until(int64_t deadline_us)
{
    for (;;) {
        esp_task_wdt_reset();
        const int64_t remaining_us = deadline_us - esp_timer_get_time();
        if (remaining_us <= 0 || atomic_load(&s_stop_requested)) {
            return;
        }

        const int64_t slice_us = remaining_us < WDT_SLICE_MS * 1000LL ? remaining_us : WDT_SLICE_MS * 1000LL;
        ulTaskNotifyTake(pdTRUE, (TickType_t)((slice_us * configTICK_RATE_HZ + 999999) / 1000000) + 1);
    }
}

static void wait_for_next_step(void)
{
    // A timeout may wake the task early, but must never call BSEC before next_call.
    const int64_t next_us = (s_bsec.next_call_ns + 999) / 1000;
    const int64_t timeout_us = s_result_deadline_ms * 1000;
    wait_until(next_us < timeout_us ? next_us : timeout_us);
}

// Saves what BSEC learned, confirms the stop and parks the task for good.
static void stop_task(void)
{
    if (s_bsec.instance != NULL && s_measurement_count > 0) {
        sensing_state_save_final(&s_bsec, &s_snapshot, &s_checkpoint, esp_timer_get_time() / 1000);
    }

    sensing_sensor_close(&s_sensor);
    ESP_LOGI(TAG, "stopped");
    esp_task_wdt_delete(NULL);
    xSemaphoreGive(s_stopped);

    for (;;) {
        vTaskDelay(portMAX_DELAY);
    }
}

// BSEC keeps its calibration across sensor failures, so only the sensor is reopened.
static esp_err_t ensure_ready(void)
{
    if (s_bsec.instance == NULL) {
        ESP_RETURN_ON_ERROR(sensing_bsec_open(&s_bsec, &s_checkpoint), TAG, "BSEC");
    }

    if (s_sensor.device == NULL) {
        ESP_RETURN_ON_ERROR(sensing_sensor_open(s_config.bus, &s_sensor), TAG, "BME680");
        s_outputs = (sensing_outputs_t){0};
        s_received = 0;
        s_result_deadline_ms = esp_timer_get_time() / 1000 + SENSING_RESULT_TIMEOUT_MS;
    }

    return ESP_OK;
}

static void log_outputs(void)
{
    if (s_measurement_count % LOG_EVERY_OUTPUTS != 1) {
        return;
    }

    ESP_LOGI(
        TAG,
        "#%lu IAQ %.1f (accuracy %u, stab %d, run-in %d), %.2f C, %.1f %%RH, %.0f Pa",
        s_measurement_count,
        s_snapshot.iaq,
        s_snapshot.iaq_accuracy,
        s_outputs.stabilized,
        s_outputs.run_in,
        s_snapshot.temperature_c,
        s_snapshot.humidity_percent,
        s_snapshot.pressure_pa);
}

static void sensing_task(void* arg)
{
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    size_t failures = 0;

    for (;;) {
        if (atomic_load(&s_stop_requested)) {
            stop_task();
        }

        uint8_t received = 0;
        esp_err_t err = ensure_ready();
        const int64_t sample_us = esp_timer_get_time();
        if (err == ESP_OK && sample_us / 1000 >= s_result_deadline_ms) {
            err = ESP_ERR_TIMEOUT;
        }

        if (err == ESP_OK && sample_us < (s_bsec.next_call_ns + 999) / 1000) {
            wait_for_next_step();

            continue;
        }

        if (err == ESP_OK) {
            err = sensing_bsec_step(&s_bsec, &s_sensor, &s_outputs, &received);
        }

        const int64_t now_ms = esp_timer_get_time() / 1000;
        if (err == ESP_OK && now_ms >= s_result_deadline_ms) {
            err = ESP_ERR_TIMEOUT;
        }

        if (err != ESP_OK) {
            const uint32_t backoff_ms = sensing_policy_retry_delay(&failures);

            ESP_LOGE(TAG, "%s; retry in %lu ms", esp_err_to_name(err), backoff_ms);
            sensing_sensor_close(&s_sensor);
            enter_error();
            wait_until(esp_timer_get_time() + backoff_ms * 1000LL);

            continue;
        }

        if (received != 0) {
            if (s_received == 0) {
                s_batch_started_ms = sample_us / 1000;
            }

            s_received |= received;
        }

        if ((s_received & SENSING_ENVIRONMENT_OUTPUTS) == SENSING_ENVIRONMENT_OUTPUTS) {
            failures = 0;
            const sensing_phase_t phase = s_snapshot.phase;
            s_snapshot = s_outputs.snapshot;
            s_measurement_count++;
            s_snapshot.phase = phase; // Keep the previous phase for transition logging.
            s_result_deadline_ms = s_batch_started_ms + SENSING_RESULT_TIMEOUT_MS;
            set_phase(sensing_policy_phase(&s_outputs));
            log_outputs();
            publish();
            sensing_state_checkpoint(&s_bsec, &s_snapshot, &s_checkpoint, now_ms);

            // Each published set needs new environment outputs. IAQ may be absent
            // during warmup; an earlier IAQ must not make the next set calibrated.
            s_received = 0;
            s_outputs.snapshot.iaq = 0;
            s_outputs.snapshot.iaq_accuracy = 0;
        }

        wait_for_next_step();
    }
}

esp_err_t sensing_start(const sensing_config_t* config)
{
    ESP_RETURN_ON_FALSE(config != NULL && config->bus != NULL, ESP_ERR_INVALID_ARG, TAG, "config");
    ESP_RETURN_ON_FALSE(s_task == NULL, ESP_ERR_INVALID_STATE, TAG, "already started");

    s_lock = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(s_lock != NULL, ESP_ERR_NO_MEM, TAG, "lock");

    esp_err_t ret = ESP_OK;
    s_stopped = xSemaphoreCreateBinary();
    ESP_GOTO_ON_FALSE(s_stopped != NULL, ESP_ERR_NO_MEM, fail, TAG, "stop semaphore");

    s_config = *config;
    atomic_store(&s_stop_requested, false);
    ESP_GOTO_ON_FALSE(
        xTaskCreate(sensing_task, "sensing", TASK_STACK_BYTES, NULL, TASK_PRIORITY, &s_task) == pdPASS,
        ESP_ERR_NO_MEM,
        fail,
        TAG,
        "task");

    return ESP_OK;

fail:
    s_task = NULL;
    if (s_stopped != NULL) {
        vSemaphoreDelete(s_stopped);
        s_stopped = NULL;
    }

    vSemaphoreDelete(s_lock);
    s_lock = NULL;
    s_config = (sensing_config_t){0};

    return ret;
}

esp_err_t sensing_stop(uint32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(s_task != NULL, ESP_ERR_INVALID_STATE, TAG, "not started");
    ESP_RETURN_ON_FALSE(!atomic_exchange(&s_stop_requested, true), ESP_ERR_INVALID_STATE, TAG, "already stopping");

    xTaskNotifyGive(s_task);
    ESP_RETURN_ON_FALSE(
        xSemaphoreTake(s_stopped, pdMS_TO_TICKS(timeout_ms)) == pdTRUE, ESP_ERR_TIMEOUT, TAG, "no confirmation");

    return ESP_OK;
}

void sensing_get_snapshot(sensing_snapshot_t* out)
{
    if (s_task == NULL) {
        *out = (sensing_snapshot_t){.phase = SENSING_PHASE_INITIALIZING};

        return;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);
    *out = s_published;
    if (out->phase != SENSING_PHASE_INITIALIZING && out->phase != SENSING_PHASE_ERROR &&
        esp_timer_get_time() / 1000 >= s_published_deadline_ms) {
        out->phase = SENSING_PHASE_ERROR;
    }

    xSemaphoreGive(s_lock);
}
