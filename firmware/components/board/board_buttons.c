#include "board_buttons.h"

#include <stdbool.h>

#include "board_pins.h"
#include "button_interface.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "iot_button.h"
#include "sdkconfig.h"

#define LONG_PRESS_MS 1500
// Smallest repeat window: the library ignores long presses after a repeat press.
#define REPEAT_WINDOW_MS 50
#define HOLD_STEP_MS CONFIG_BUTTON_LONG_PRESS_HOLD_SERIAL_TIME_MS

_Static_assert((CONFIG_BUTTON_PERIOD_TIME_MS * CONFIG_BUTTON_DEBOUNCE_TICKS) == 50, "debounce must be 50 ms");
_Static_assert(HOLD_STEP_MS > 0 && HOLD_STEP_MS < LONG_PRESS_MS, "hold step must fit the long press");

typedef struct {
    button_driver_t driver; // First member: library callbacks borrow this static slot.
    gpio_num_t pin;
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

static uint8_t get_key_level(button_driver_t* driver)
{
    const button_slot_t* slot = (const button_slot_t*)driver;

    return gpio_get_level(slot->pin) == 0;
}

static int32_t get_gpio_num(button_driver_t* driver)
{
    const button_slot_t* slot = (const button_slot_t*)driver;

    return slot->pin;
}

static esp_err_t enter_power_save(button_driver_t* driver)
{
    const button_slot_t* slot = (const button_slot_t*)driver;
    ESP_RETURN_ON_ERROR(gpio_wakeup_enable(slot->pin, GPIO_INTR_LOW_LEVEL), TAG, "wakeup on");

    return gpio_intr_enable(slot->pin);
}

static esp_err_t exit_power_save(button_driver_t* driver)
{
    const button_slot_t* slot = (const button_slot_t*)driver;
    ESP_RETURN_ON_ERROR(gpio_intr_disable(slot->pin), TAG, "interrupt off");

    return gpio_wakeup_disable(slot->pin);
}

static esp_err_t delete_gpio_driver(button_driver_t* driver)
{
    const button_slot_t* slot = (const button_slot_t*)driver;
    ESP_RETURN_ON_ERROR(exit_power_save(driver), TAG, "wakeup off");
    ESP_RETURN_ON_ERROR(gpio_isr_handler_remove(slot->pin), TAG, "GPIO handler remove");

    return gpio_reset_pin(slot->pin);
}

static void IRAM_ATTR on_gpio_wakeup(void* arg)
{
    iot_button_power_save_wakeup_isr((uint32_t)(uintptr_t)arg);
}

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

    if (slot->long_sent) {
        return;
    }

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
        // HOLD first fires one step after START. Avoid START: button 4.2.2 reads
        // past its callback array on the next tick after the last START callback.
        .long_press_time = LONG_PRESS_MS - HOLD_STEP_MS,
        .short_press_time = REPEAT_WINDOW_MS,
    };
    const gpio_num_t pin = board_button_pin(button);
    const gpio_config_t pin_config = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        // Same pull-up as board_init_safe_gpio: only where the pin has one.
        .pull_up_en = board_button_internal_pull_up(button) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&pin_config), TAG, "GPIO config");
    ESP_RETURN_ON_ERROR(gpio_wakeup_enable(pin, GPIO_INTR_LOW_LEVEL), TAG, "GPIO wakeup");

    button_slot_t* slot = &s_slots[button];
    slot->pin = pin;
    slot->driver = (button_driver_t){
        .enable_power_save = true,
        .get_key_level = get_key_level,
        .enter_power_save = enter_power_save,
        .exit_power_save = exit_power_save,
        .get_gpio_num = get_gpio_num,
        .del = delete_gpio_driver,
    };

    slot->held_at_init = gpio_get_level(pin) == 0;
    ESP_RETURN_ON_ERROR(iot_button_create(&config, &slot->driver, &slot->handle), TAG, "create");
    ESP_RETURN_ON_ERROR(gpio_set_intr_type(pin, GPIO_INTR_LOW_LEVEL), TAG, "interrupt type");
    ESP_RETURN_ON_ERROR(gpio_isr_handler_add(pin, on_gpio_wakeup, (void*)(uintptr_t)pin), TAG, "GPIO handler");

    void* arg = (void*)(intptr_t)button;
    ESP_RETURN_ON_ERROR(
        iot_button_register_cb(slot->handle, BUTTON_PRESS_DOWN, NULL, on_press_down, arg), TAG, "press down");
    ESP_RETURN_ON_ERROR(
        iot_button_register_cb(slot->handle, BUTTON_LONG_PRESS_HOLD, NULL, on_long_press, arg), TAG, "long press");

    return iot_button_register_cb(slot->handle, BUTTON_PRESS_UP, NULL, on_press_up, arg);
}

esp_err_t board_buttons_init(board_button_sink_fn sink, void* ctx)
{
    ESP_RETURN_ON_FALSE(sink != NULL, ESP_ERR_INVALID_ARG, TAG, "sink");
    ESP_RETURN_ON_FALSE(s_sink == NULL, ESP_ERR_INVALID_STATE, TAG, "already initialized");

    ESP_RETURN_ON_ERROR(esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "buttons", &s_pm_lock), TAG, "PM lock");
    // Both buttons share this service for the entire boot; each gets its own handler.
    ESP_RETURN_ON_ERROR(gpio_install_isr_service(ESP_INTR_FLAG_IRAM), TAG, "GPIO ISR service");
    ESP_RETURN_ON_ERROR(esp_sleep_enable_gpio_wakeup(), TAG, "GPIO wakeup source");
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
