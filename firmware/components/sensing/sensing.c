#include "sensing.h"

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

static const uint32_t BACKOFF_MS[] = {1000, 2000, 5000, 10000, 30000};

#define BACKOFF_COUNT (sizeof(BACKOFF_MS) / sizeof(BACKOFF_MS[0]))

static sensing_config_t s_config;
static TaskHandle_t s_task;
static SemaphoreHandle_t s_lock;
static sensing_snapshot_t s_published;

static sensing_snapshot_t s_snapshot; // Task-owned working copy.
static sensing_sensor_t s_sensor;
static sensing_bsec_t s_bsec;

static const char* phase_name(sensing_phase_t phase)
{
    static const char* const NAMES[] = {"initializing", "error", "warmup", "calibrating", "ready"};

    return NAMES[phase];
}

static sensing_phase_t phase_of(const sensing_snapshot_t* snapshot)
{
    if (!snapshot->iaq_valid || !snapshot->stabilized || !snapshot->run_in || snapshot->iaq_accuracy == 0) {
        return SENSING_PHASE_WARMUP;
    }

    return snapshot->iaq_accuracy == 1 ? SENSING_PHASE_CALIBRATING : SENSING_PHASE_READY;
}

static void publish(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_published = s_snapshot;
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

// Values from before a failure are not current measurements.
static void enter_error(void)
{
    s_snapshot.iaq_valid = false;
    s_snapshot.temperature_valid = false;
    s_snapshot.humidity_valid = false;
    s_snapshot.pressure_valid = false;
    set_phase(SENSING_PHASE_ERROR);
    publish();
}

static void wait_ms(int64_t duration_ms)
{
    while (duration_ms > 0) {
        const int64_t slice = duration_ms < WDT_SLICE_MS ? duration_ms : WDT_SLICE_MS;
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(slice) > 0 ? pdMS_TO_TICKS(slice) : 1);
        duration_ms -= slice;
    }

    esp_task_wdt_reset();
}

// BSEC keeps its calibration across sensor failures, so only the sensor is reopened.
static esp_err_t ensure_ready(void)
{
    if (s_bsec.instance == NULL) {
        ESP_RETURN_ON_ERROR(sensing_bsec_open(&s_bsec), TAG, "BSEC");
    }

    if (s_sensor.device == NULL) {
        ESP_RETURN_ON_ERROR(sensing_sensor_open(s_config.bus, &s_sensor), TAG, "BME680");
        set_phase(SENSING_PHASE_WARMUP);
        publish();
    }

    return ESP_OK;
}

static void log_outputs(void)
{
    if (s_snapshot.generation % LOG_EVERY_OUTPUTS != 1) {
        return;
    }

    ESP_LOGI(TAG,
        "#%lu static IAQ %.1f (accuracy %u, stab %d, run-in %d), %.2f C, %.1f %%RH, %.0f Pa",
        s_snapshot.generation,
        s_snapshot.static_iaq,
        s_snapshot.iaq_accuracy,
        s_snapshot.stabilized,
        s_snapshot.run_in,
        s_snapshot.temperature_c,
        s_snapshot.humidity_percent,
        s_snapshot.pressure_pa);
}

static void sensing_task(void* arg)
{
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    size_t failures = 0;

    for (;;) {
        bool updated = false;
        esp_err_t err = ensure_ready();
        if (err == ESP_OK) {
            err = sensing_bsec_step(&s_bsec, &s_sensor, &s_snapshot, &updated);
        }

        if (err != ESP_OK) {
            const uint32_t backoff_ms = BACKOFF_MS[failures];
            if (failures < BACKOFF_COUNT - 1) {
                failures++;
            }

            ESP_LOGE(TAG, "%s; retry in %lu ms", esp_err_to_name(err), backoff_ms);
            sensing_sensor_close(&s_sensor);
            enter_error();
            wait_ms(backoff_ms);

            continue;
        }

        failures = 0;
        if (updated) {
            s_snapshot.generation++;
            s_snapshot.updated_ms = esp_timer_get_time() / 1000;
            set_phase(phase_of(&s_snapshot));
            log_outputs();
            publish();
        }

        wait_ms((s_bsec.next_call_ns - esp_timer_get_time() * 1000) / 1000000);
    }
}

esp_err_t sensing_start(const sensing_config_t* config)
{
    ESP_RETURN_ON_FALSE(config != NULL && config->bus != NULL, ESP_ERR_INVALID_ARG, TAG, "config");
    ESP_RETURN_ON_FALSE(s_task == NULL, ESP_ERR_INVALID_STATE, TAG, "already started");

    s_config = *config;
    s_lock = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(s_lock != NULL, ESP_ERR_NO_MEM, TAG, "lock");

    ESP_RETURN_ON_FALSE(xTaskCreate(sensing_task, "sensing", TASK_STACK_BYTES, NULL, TASK_PRIORITY, &s_task) == pdPASS,
        ESP_ERR_NO_MEM,
        TAG,
        "task");

    return ESP_OK;
}

void sensing_get_snapshot(sensing_snapshot_t* out)
{
    if (s_lock == NULL) {
        *out = (sensing_snapshot_t){.phase = SENSING_PHASE_INITIALIZING};

        return;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);
    *out = s_published;
    xSemaphoreGive(s_lock);
}
