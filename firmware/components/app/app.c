#include "app.h"

#include <stdatomic.h>

#include "app_state.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "power.h"
#include "sensing.h"
#include "storage.h"
#include "ui.h"

#define INPUT_QUEUE_LENGTH 16
#define TASK_STACK_BYTES 4096
#define TASK_PRIORITY 5
#define UI_RETRY_MS 100
#define SENSING_STOP_TIMEOUT_MS 3000 // A stuck sensor must not keep the device awake.
// Longer waits run without the 10 s task watchdog, so monitoring needs no wake-ups
// just to feed it; a shorter wait and the work after it stay within the timeout.
#define WDT_WATCHED_WAIT_MS 5000

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

static uint32_t handle_input(void)
{
    app_button_event_t events[INPUT_QUEUE_LENGTH];
    size_t count = 0;
    while (count < INPUT_QUEUE_LENGTH && xQueueReceive(s_input, &events[count], 0) == pdTRUE) {
        count++;
    }

    // Validate the whole batch before a gesture can confirm shutdown. Events
    // arriving after this check belong to the next batch and notify the task.
    if (atomic_exchange(&s_input_lost, false)) {
        xQueueReset(s_input);
        ESP_LOGE(TAG, "button queue overflowed; queued input discarded");

        return app_state_input_lost(&s_state);
    }

    uint32_t effects = APP_EFFECT_NONE;
    for (size_t i = 0; i < count; i++) {
        effects |= app_state_button(&s_state, &events[i]);
    }

    return effects;
}

// Buttons act at their own timestamps; everything else at now.
static uint32_t handle_notifications(uint32_t bits, int64_t now)
{
    uint32_t effects = bits & NOTIFY_INPUT ? handle_input() : APP_EFFECT_NONE;

    // Notifications coalesce; only the latest snapshot matters.
    if (bits & NOTIFY_MEASUREMENT) {
        sensing_snapshot_t measurement;
        sensing_get_snapshot(&measurement);
        effects |= app_state_measurement(&s_state, &measurement);
    }

    return effects | app_state_tick(&s_state, now);
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

// Reads the battery when the state asks for it; the result may start a power-off.
static uint32_t poll_power(uint32_t effects, int64_t now)
{
    if (!(effects & APP_EFFECT_POLL_POWER)) {
        return effects;
    }

    power_status_t status;
    power_poll(&status);

    return effects | app_state_power(&s_state, &status, now);
}

// Settings are already saved by the same transition. No storage write follows a
// sensing stop that was not confirmed, so the two cannot interleave.
__attribute__((noreturn)) static void shut_down(void)
{
    static const char* const NAMES[] = {
        [APP_SHUTDOWN_USER] = "confirmed",
        [APP_SHUTDOWN_CRITICAL] = "battery is flat",
        [APP_SHUTDOWN_FAULT] = "display cannot turn off",
    };
    static const power_sleep_reason_t SLEEP_REASONS[] = {
        [APP_SHUTDOWN_USER] = POWER_SLEEP_USER,
        [APP_SHUTDOWN_CRITICAL] = POWER_SLEEP_CRITICAL,
        [APP_SHUTDOWN_FAULT] = POWER_SLEEP_FAULT,
    };

    const app_shutdown_reason_t reason = s_state.shutdown_reason;
    if (reason == APP_SHUTDOWN_FAULT) {
        ESP_LOGE(TAG, "power-off: %s", NAMES[reason]);
    } else {
        ESP_LOGI(TAG, "power-off: %s", NAMES[reason]);
    }

    // Dark before the sensor stops. If the panel cannot sleep, the final GPIO path
    // holds the backlight LOW and resets the panel instead.
    esp_err_t err = ui_set_display_state(UI_DISPLAY_OFF, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "UI off: %s; the sleep path resets the panel instead", esp_err_to_name(err));
    }

    err = sensing_stop(SENSING_STOP_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "sensing stop: %s; sleeping without its last checkpoint", esp_err_to_name(err));
    }

    power_enter_sleep(SLEEP_REASONS[reason]);
}

static uint32_t update_display(int64_t now)
{
    static const char* const NAMES[] = {
        [UI_DISPLAY_ON] = "ON",
        [UI_DISPLAY_DIM] = "DIM",
        [UI_DISPLAY_OFF] = "OFF",
    };

    const app_mode_t mode = s_state.mode;
    const esp_err_t err = ui_set_display_state(s_state.display, app_state_backlight(&s_state));

    // Without a display there is nothing to light or darken.
    const bool reached = err == ESP_OK || err == ESP_ERR_INVALID_STATE;
    if (!reached) {
        ESP_LOGE(
            TAG,
            "display %s failed: %s (attempt %u of %u)",
            NAMES[s_state.display],
            esp_err_to_name(err),
            s_state.display_failures + 1,
            APP_DISPLAY_RETRIES + 1);
    }

    const uint32_t effects = app_state_display_result(&s_state, reached, now);
    if (s_state.mode == APP_MODE_MONITORING && mode != APP_MODE_MONITORING) {
        ESP_LOGI(TAG, "monitoring");
    }

    return effects;
}

static void carry_out(uint32_t effects, int64_t now)
{
    // The result may start a power-off, which the steps below carry out.
    if (effects & APP_EFFECT_DISPLAY) {
        effects |= update_display(now);
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
        shut_down();
    }
}

// Rounds up and adds one tick: a wait of n ticks can end right after the next
// tick boundary, so n = ceil(remaining) alone could wake before the deadline.
static TickType_t ticks_until(int64_t deadline_ms)
{
    if (deadline_ms == APP_NO_DEADLINE) {
        return portMAX_DELAY;
    }

    const int64_t remaining = deadline_ms - now_ms();

    return remaining <= 0 ? 0 : (TickType_t)((remaining * configTICK_RATE_HZ + 999) / 1000) + 1;
}

static void app_task(void* arg)
{
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    int64_t now = now_ms();
    uint32_t effects = poll_power(app_state_activate(&s_state, now), now);
    bool model_dirty = false;

    for (;;) {
        if (effects & APP_EFFECT_MODEL) {
            model_dirty = true;
        }

        // A dark display takes the model on its next wake. The model goes out before
        // the display request, so the first frame after OFF already shows it.
        if (model_dirty && s_state.display != UI_DISPLAY_OFF) {
            model_dirty = !apply_model();
        }

        carry_out(effects, now);

        int64_t deadline = app_state_next_deadline(&s_state);
        const int64_t retry_ms = now_ms() + UI_RETRY_MS;
        if (model_dirty && s_state.display != UI_DISPLAY_OFF && retry_ms < deadline) {
            deadline = retry_ms;
        }

        const TickType_t ticks = ticks_until(deadline);
        const bool unwatched = ticks > pdMS_TO_TICKS(WDT_WATCHED_WAIT_MS);
        if (unwatched) {
            ESP_ERROR_CHECK(esp_task_wdt_delete(NULL));
        }

        uint32_t bits = 0;
        xTaskNotifyWait(0, UINT32_MAX, &bits, ticks);

        if (unwatched) {
            ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
        } else {
            esp_task_wdt_reset();
        }

        // One reading for the iteration, so all its transitions see the same time.
        now = now_ms();
        effects = poll_power(handle_notifications(bits, now), now);
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

    atomic_store(&s_input_lost, false);
    if (xTaskCreate(app_task, "app", TASK_STACK_BYTES, NULL, TASK_PRIORITY, &s_task) != pdPASS) {
        s_task = NULL;
        vQueueDelete(s_input);
        s_input = NULL;
        ESP_LOGE(TAG, "task creation failed");

        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
