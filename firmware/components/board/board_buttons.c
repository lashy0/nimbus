#include "board_buttons.h"

#include <stdbool.h>

#include "board_pins.h"
#include "button_gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_timer.h"
#include "iot_button.h"
#include "sdkconfig.h"

#define LONG_PRESS_MS 1500
// Smallest repeat window: the library ignores long presses after a repeat press.
#define REPEAT_WINDOW_MS 50

_Static_assert((CONFIG_BUTTON_PERIOD_TIME_MS * CONFIG_BUTTON_DEBOUNCE_TICKS) == 50, "debounce must be 50 ms");

typedef struct {
    button_handle_t handle;
    bool pressed;
    bool long_sent;
    bool held_at_init; // Wake press: consumed until its release.
} button_slot_t;

static const char* TAG = "board_buttons";

static board_button_sink_fn s_sink;
static void* s_sink_ctx;
static esp_pm_lock_handle_t s_pm_lock;
static button_slot_t s_slots[BOARD_BUTTON_COUNT];

// Library callbacks all run in the esp_timer task, so slot state needs no lock.
static void emit(board_button_t button, board_button_event_type_t type)
{
    const board_button_event_t event = {
        .button = button,
        .type = type,
        .timestamp_us = esp_timer_get_time(),
    };

    s_sink(s_sink_ctx, &event);
}

static void on_press_down(void* handle, void* arg)
{
    const board_button_t button = (board_button_t)(intptr_t)arg;
    button_slot_t* slot = &s_slots[button];

    if (!slot->pressed) {
        esp_err_t err = esp_pm_lock_acquire(s_pm_lock);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "PM lock acquire: %s", esp_err_to_name(err));
        }

        slot->pressed = true;
    }

    slot->long_sent = false;
    emit(button, BOARD_BUTTON_PRESSED);
}

static void on_long_press(void* handle, void* arg)
{
    const board_button_t button = (board_button_t)(intptr_t)arg;
    button_slot_t* slot = &s_slots[button];

    slot->long_sent = true;
    if (!slot->held_at_init) {
        emit(button, BOARD_BUTTON_LONG);
    }
}

static void on_press_up(void* handle, void* arg)
{
    const board_button_t button = (board_button_t)(intptr_t)arg;
    button_slot_t* slot = &s_slots[button];

    if (!slot->long_sent && !slot->held_at_init) {
        emit(button, BOARD_BUTTON_SHORT);
    }

    slot->held_at_init = false;
    emit(button, BOARD_BUTTON_RELEASED);

    if (slot->pressed) {
        slot->pressed = false;
        esp_err_t err = esp_pm_lock_release(s_pm_lock);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "PM lock release: %s", esp_err_to_name(err));
        }
    }
}

static esp_err_t create_button(board_button_t button)
{
    const button_config_t config = {
        .long_press_time = LONG_PRESS_MS,
        .short_press_time = REPEAT_WINDOW_MS,
    };
    const gpio_num_t pin = board_button_pin(button);
    const button_gpio_config_t gpio_config = {
        .gpio_num = pin,
        .active_level = 0,
        .enable_power_save = true,
        // Same pull-up as board_init_safe_gpio: only where the pin has one.
        .disable_pull = !board_button_internal_pull_up(button),
    };
    button_slot_t* slot = &s_slots[button];

    slot->held_at_init = gpio_get_level(pin) == 0;
    ESP_RETURN_ON_ERROR(iot_button_new_gpio_device(&config, &gpio_config, &slot->handle), TAG, "create");

    void* arg = (void*)(intptr_t)button;
    ESP_RETURN_ON_ERROR(
        iot_button_register_cb(slot->handle, BUTTON_PRESS_DOWN, NULL, on_press_down, arg), TAG, "press down");
    ESP_RETURN_ON_ERROR(
        iot_button_register_cb(slot->handle, BUTTON_LONG_PRESS_START, NULL, on_long_press, arg), TAG, "long press");

    return iot_button_register_cb(slot->handle, BUTTON_PRESS_UP, NULL, on_press_up, arg);
}

esp_err_t board_buttons_init(board_button_sink_fn sink, void* ctx)
{
    ESP_RETURN_ON_FALSE(sink != NULL, ESP_ERR_INVALID_ARG, TAG, "sink");
    ESP_RETURN_ON_FALSE(s_sink == NULL, ESP_ERR_INVALID_STATE, TAG, "already initialized");

    ESP_RETURN_ON_ERROR(esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "buttons", &s_pm_lock), TAG, "PM lock");
    s_sink = sink;
    s_sink_ctx = ctx;

    // A held button's interrupt starts library polling immediately, so a created button
    // cannot be torn down safely here; it stays active if a later one fails.
    for (unsigned i = 0; i < BOARD_BUTTON_COUNT; ++i) {
        ESP_RETURN_ON_ERROR(create_button((board_button_t)i), TAG, "button %u", i);
    }

    ESP_LOGI(
        TAG,
        "ready, held at init: PREV=%d NEXT=%d",
        s_slots[BOARD_BUTTON_PREV].held_at_init,
        s_slots[BOARD_BUTTON_NEXT].held_at_init);

    return ESP_OK;
}
