#include "app.h"

#include <stdatomic.h>

#include "app_state.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sensing.h"
#include "storage.h"
#include "ui.h"

#define INPUT_QUEUE_LENGTH 16
#define TASK_STACK_BYTES 4096
#define TASK_PRIORITY 5
#define UI_RETRY_MS 100

#define NOTIFY_INPUT (1u << 0)
#define NOTIFY_MEASUREMENT (1u << 1)

static const char* TAG = "app";

static app_state_t s_state;
static TaskHandle_t s_task;
static QueueHandle_t s_input;
static atomic_bool s_input_lost;

static int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

void app_post_button(const app_button_event_t* event)
{
    if (s_task == NULL) {
        return;
    }

    if (xQueueSend(s_input, event, 0) != pdTRUE) {
        atomic_store(&s_input_lost, true);
    }

    xTaskNotify(s_task, NOTIFY_INPUT, eSetBits);
}

void app_notify_measurement(void)
{
    if (s_task != NULL) {
        xTaskNotify(s_task, NOTIFY_MEASUREMENT, eSetBits);
    }
}

static uint32_t handle_notifications(uint32_t bits)
{
    uint32_t effects = APP_EFFECT_NONE;
    if (bits & NOTIFY_INPUT) {
        app_button_event_t event;
        while (xQueueReceive(s_input, &event, 0) == pdTRUE) {
            effects |= app_state_button(&s_state, &event, now_ms());
        }

        if (atomic_exchange(&s_input_lost, false)) {
            ESP_LOGE(TAG, "button queue overflowed; input lost");
            effects |= app_state_cancel_dialog(&s_state);
        }
    }

    // Notifications coalesce; only the latest snapshot matters.
    if (bits & NOTIFY_MEASUREMENT) {
        sensing_snapshot_t measurement;
        sensing_get_snapshot(&measurement);
        effects |= app_state_measurement(&s_state, &measurement, now_ms());
    }

    return effects | app_state_tick(&s_state, now_ms());
}

// Returns false when the model must be sent again later.
static bool apply_model(void)
{
    ui_model_t model;
    app_state_model(&s_state, &model);

    const esp_err_t err = ui_apply_model(&model);
    if (err == ESP_ERR_TIMEOUT) {
        return false;
    }

    // ESP_ERR_INVALID_STATE: no display; the device keeps working without it.
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "UI update failed: %s", esp_err_to_name(err));
    }

    return true;
}

static void carry_out(uint32_t effects)
{
    if (effects & APP_EFFECT_BRIGHTNESS) {
        const esp_err_t err = ui_set_display_state(UI_DISPLAY_ON, s_state.brightness_percent);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "brightness %u%%: %s", s_state.brightness_percent, esp_err_to_name(err));
        }
    }

    // A failed save keeps the previous value; the next change tries again.
    if (effects & APP_EFFECT_SAVE_SETTINGS) {
        const storage_settings_t settings = {.brightness_percent = s_state.brightness_percent};
        const esp_err_t err = storage_save_settings(&settings);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "brightness %u%% saved", settings.brightness_percent);
        } else {
            ESP_LOGW(TAG, "brightness not saved: %s", esp_err_to_name(err));
        }
    }

    if (effects & APP_EFFECT_SHUTDOWN) {
        ESP_LOGW(TAG, "power-off confirmed, but there is no power component yet; staying on");
    }
}

static TickType_t ticks_until(int64_t deadline_ms)
{
    if (deadline_ms == APP_NO_DEADLINE) {
        return portMAX_DELAY;
    }

    const int64_t remaining = deadline_ms - now_ms();

    return remaining <= 0 ? 0 : pdMS_TO_TICKS(remaining) + 1;
}

static void app_task(void* arg)
{
    uint32_t effects = app_state_activate(&s_state);
    bool model_dirty = false;

    for (;;) {
        carry_out(effects);
        if (effects & APP_EFFECT_MODEL) {
            model_dirty = true;
        }

        if (model_dirty) {
            model_dirty = !apply_model();
        }

        int64_t deadline = app_state_next_deadline(&s_state);
        if (model_dirty && now_ms() + UI_RETRY_MS < deadline) {
            deadline = now_ms() + UI_RETRY_MS;
        }

        uint32_t bits = 0;
        xTaskNotifyWait(0, UINT32_MAX, &bits, ticks_until(deadline));
        effects = handle_notifications(bits);
    }
}

uint8_t app_load_brightness(void)
{
    storage_settings_t settings;
    const esp_err_t err = storage_load_settings(&settings);
    if (err == ESP_OK && app_brightness_valid(settings.brightness_percent)) {
        ESP_LOGI(TAG, "saved brightness %u%%", settings.brightness_percent);

        return settings.brightness_percent;
    }

    if (err == ESP_OK) {
        ESP_LOGW(TAG, "saved brightness %u%% is out of range", settings.brightness_percent);
    } else if (err != ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "saved settings unavailable: %s", esp_err_to_name(err));
    }

    return APP_BRIGHTNESS_DEFAULT;
}

esp_err_t app_start(const app_config_t* config)
{
    ESP_RETURN_ON_FALSE(
        config != NULL && config->brightness_percent >= APP_BRIGHTNESS_MIN &&
            config->brightness_percent <= APP_BRIGHTNESS_MAX,
        ESP_ERR_INVALID_ARG,
        TAG,
        "config");
    ESP_RETURN_ON_FALSE(s_task == NULL, ESP_ERR_INVALID_STATE, TAG, "already started");

    app_state_init(&s_state, config->brightness_percent);

    s_input = xQueueCreate(INPUT_QUEUE_LENGTH, sizeof(app_button_event_t));
    ESP_RETURN_ON_FALSE(s_input != NULL, ESP_ERR_NO_MEM, TAG, "input queue");

    ESP_RETURN_ON_FALSE(
        xTaskCreate(app_task, "app", TASK_STACK_BYTES, NULL, TASK_PRIORITY, &s_task) == pdPASS,
        ESP_ERR_NO_MEM,
        TAG,
        "task");

    return ESP_OK;
}
