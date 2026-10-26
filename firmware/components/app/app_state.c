#include "app_state.h"

#include <string.h>

#include "app_internal.h"

bool app_is_main_screen(ui_screen_t screen)
{
    return screen == UI_SCREEN_IAQ || screen == UI_SCREEN_TEMP || screen == UI_SCREEN_HUM;
}

// An error ends the wait as well: retries may take far longer than the first output.
static bool first_measurement_arrived(const sensing_snapshot_t* measurement)
{
    return measurement->phase != SENSING_PHASE_INITIALIZING;
}

// Working means fresh temperature and humidity; IAQ may still be calibrating.
app_sensor_status_t app_sensor_status(const app_state_t* state)
{
    if (state->first_measurement_deadline_ms != APP_NO_DEADLINE) {
        return APP_SENSOR_WAITING;
    }

    const sensing_snapshot_t* measurement = &state->measurement;
    const bool running = measurement->phase == SENSING_PHASE_WARMUP ||
                         measurement->phase == SENSING_PHASE_CALIBRATING || measurement->phase == SENSING_PHASE_READY;

    return running ? APP_SENSOR_WORKING : APP_SENSOR_FAULT;
}

// NEXT walks IAQ -> TEMP -> HUM -> IAQ, PREV the other way.
static ui_screen_t step_main_screen(ui_screen_t screen, app_button_t button)
{
    static const ui_screen_t ORDER[] = {UI_SCREEN_IAQ, UI_SCREEN_TEMP, UI_SCREEN_HUM};
    const size_t count = sizeof(ORDER) / sizeof(ORDER[0]);

    size_t index = 0;
    while (ORDER[index] != screen) {
        index++;
    }

    index = button == APP_BUTTON_NEXT ? (index + 1) % count : (index + count - 1) % count;

    return ORDER[index];
}

static uint32_t close_dialog(app_state_t* state)
{
    state->screen = state->main_screen;
    state->dialog_deadline_ms = APP_NO_DEADLINE;

    return APP_EFFECT_MODEL;
}

static uint32_t open_shutdown_question(app_state_t* state, int64_t now_ms)
{
    state->screen = UI_SCREEN_SHUTDOWN;
    state->shutdown_yes_selected = true;
    state->dialog_deadline_ms = now_ms + APP_DIALOG_TIMEOUT_MS;

    return APP_EFFECT_MODEL;
}

uint32_t app_save_settings(app_state_t* state)
{
    if (state->settings_deadline_ms == APP_NO_DEADLINE) {
        return APP_EFFECT_NONE;
    }

    state->settings_deadline_ms = APP_NO_DEADLINE;

    return APP_EFFECT_SAVE_SETTINGS;
}

// The pending settings are saved first; input stays ignored from here on.
uint32_t app_begin_shutdown(app_state_t* state, app_shutdown_reason_t reason)
{
    state->mode = APP_MODE_SHUTTING_DOWN;
    state->shutdown_reason = reason;
    state->dialog_deadline_ms = APP_NO_DEADLINE;

    return app_save_settings(state) | APP_EFFECT_SHUTDOWN;
}

static uint32_t change_brightness(app_state_t* state, int delta, int64_t now_ms)
{
    int percent = state->brightness_percent + delta;
    if (percent < APP_BRIGHTNESS_MIN) {
        percent = APP_BRIGHTNESS_MIN;
    } else if (percent > APP_BRIGHTNESS_MAX) {
        percent = APP_BRIGHTNESS_MAX;
    }

    if (percent == state->brightness_percent) {
        return APP_EFFECT_NONE;
    }

    state->brightness_percent = (uint8_t)percent;
    state->settings_deadline_ms = now_ms + APP_SETTINGS_SAVE_DELAY_MS;

    return APP_EFFECT_MODEL | APP_EFFECT_DISPLAY;
}

static uint32_t on_short(app_state_t* state, app_button_t button, int64_t now_ms)
{
    switch (state->screen) {
        case UI_SCREEN_IAQ:
        case UI_SCREEN_TEMP:
        case UI_SCREEN_HUM:
            // The start screen and the error page stand for all three pages.
            if (app_sensor_status(state) != APP_SENSOR_WORKING) {
                return APP_EFFECT_NONE;
            }

            state->screen = step_main_screen(state->screen, button);
            state->main_screen = state->screen;

            return APP_EFFECT_MODEL;

        case UI_SCREEN_BRIGHTNESS:
            return change_brightness(
                state, button == APP_BUTTON_NEXT ? APP_BRIGHTNESS_STEP : -APP_BRIGHTNESS_STEP, now_ms);

        case UI_SCREEN_SHUTDOWN:
            // A press on the highlighted answer chooses it; otherwise it moves the highlight.
            if (button == APP_BUTTON_PREV && state->shutdown_yes_selected) {
                return close_dialog(state) | app_begin_shutdown(state, APP_SHUTDOWN_USER);
            }

            if (button == APP_BUTTON_NEXT && !state->shutdown_yes_selected) {
                return close_dialog(state);
            }

            state->shutdown_yes_selected = button == APP_BUTTON_PREV;

            return APP_EFFECT_MODEL;

        case UI_SCREEN_START:
        case UI_SCREEN_SENSOR_ERROR:
            break;
    }

    return APP_EFFECT_NONE;
}

static uint32_t on_long(app_state_t* state, app_button_t button, int64_t now_ms)
{
    if (app_is_main_screen(state->screen)) {
        if (button == APP_BUTTON_PREV) {
            return open_shutdown_question(state, now_ms);
        }

        state->screen = UI_SCREEN_BRIGHTNESS;

        return APP_EFFECT_MODEL;
    }

    // Leaving the screen saves a pending change without waiting for the delay.
    if (state->screen == UI_SCREEN_BRIGHTNESS && button == APP_BUTTON_NEXT) {
        state->screen = state->main_screen;

        return APP_EFFECT_MODEL | app_save_settings(state);
    }

    return APP_EFFECT_NONE;
}

bool app_brightness_valid(uint8_t percent)
{
    return percent >= APP_BRIGHTNESS_MIN && percent <= APP_BRIGHTNESS_MAX && percent % APP_BRIGHTNESS_STEP == 0;
}

void app_state_init(app_state_t* state, uint8_t brightness_percent)
{
    memset(state, 0, sizeof(*state));
    state->mode = APP_MODE_STARTING;
    state->screen = UI_SCREEN_START;
    state->main_screen = UI_SCREEN_IAQ;
    state->brightness_percent = brightness_percent;
    state->settings_deadline_ms = APP_NO_DEADLINE;
    state->dialog_deadline_ms = APP_NO_DEADLINE;
    state->first_measurement_deadline_ms = APP_NO_DEADLINE; // Counts from activation.
    state->power.source = POWER_SOURCE_UNKNOWN;
    state->power_deadline_ms = APP_NO_DEADLINE; // Polling starts with activation.
    state->notice_deadline_ms = APP_NO_DEADLINE;
    state->display = UI_DISPLAY_ON; // ui_init() leaves the display lit.
    state->display_retry_ms = APP_NO_DEADLINE;
}

uint32_t app_state_activate(app_state_t* state, int64_t now_ms)
{
    if (state->mode != APP_MODE_STARTING) {
        return APP_EFFECT_NONE;
    }

    state->mode = APP_MODE_ACTIVE;
    state->screen = state->main_screen;
    state->activity_ms = now_ms;
    if (!first_measurement_arrived(&state->measurement)) {
        state->first_measurement_deadline_ms = now_ms + APP_FIRST_MEASUREMENT_MS;
    }

    return APP_EFFECT_MODEL | APP_EFFECT_POLL_POWER;
}

uint32_t app_state_button(app_state_t* state, const app_button_event_t* event)
{
    if (state->mode != APP_MODE_ACTIVE && state->mode != APP_MODE_MONITORING) {
        return APP_EFFECT_NONE;
    }

    const int64_t now_ms = event->timestamp_ms;

    // A press at a deadline already belongs to what the deadline starts.
    uint32_t effects = app_state_tick(state, now_ms);
    bool consumed;
    effects |= app_display_button(state, event, &consumed);
    if (consumed) {
        return effects;
    }

    if (event->gesture == APP_GESTURE_SHORT) {
        return effects | on_short(state, event->button, now_ms);
    }

    return effects | on_long(state, event->button, now_ms);
}

uint32_t app_state_measurement(app_state_t* state, const sensing_snapshot_t* measurement)
{
    state->measurement = *measurement;

    if (first_measurement_arrived(measurement)) {
        state->first_measurement_deadline_ms = APP_NO_DEADLINE;
    }

    return APP_EFFECT_MODEL;
}

uint32_t app_state_power(app_state_t* state, const power_status_t* power, int64_t now_ms)
{
    state->power_deadline_ms = now_ms + APP_POWER_POLL_MS;
    if (state->mode == APP_MODE_SHUTTING_DOWN) {
        return APP_EFFECT_NONE;
    }

    const power_status_t* shown = &state->power;
    const bool changed = power->source != shown->source || power->percent_valid != shown->percent_valid ||
                         power->percent != shown->percent;
    uint32_t effects = changed ? APP_EFFECT_MODEL : APP_EFFECT_NONE;

    effects |= app_display_power(state, power, now_ms);
    state->power = *power;

    if (power->critical) {
        effects |= app_state_cancel_dialog(state) | app_begin_shutdown(state, APP_SHUTDOWN_CRITICAL);
    }

    return effects;
}

// The order also defines timeout handling when several deadlines coincide.
typedef enum {
    APP_TIMER_POWER,
    APP_TIMER_FIRST_MEASUREMENT,
    APP_TIMER_DIALOG,
    APP_TIMER_SETTINGS,
    APP_TIMER_USB_NOTICE,
    APP_TIMER_DIM,
    APP_TIMER_OFF,
    APP_TIMER_DISPLAY_RETRY,
    APP_TIMER_COUNT,
} app_timer_t;

// Both timeout handling and task scheduling use the same enabled deadlines.
static int64_t timer_deadline(const app_state_t* state, app_timer_t timer)
{
    switch (timer) {
        case APP_TIMER_POWER:
            return state->power_deadline_ms;
        case APP_TIMER_FIRST_MEASUREMENT:
            return state->first_measurement_deadline_ms;
        case APP_TIMER_DIALOG:
            return state->screen == UI_SCREEN_SHUTDOWN ? state->dialog_deadline_ms : APP_NO_DEADLINE;
        case APP_TIMER_SETTINGS:
            return state->settings_deadline_ms;
        case APP_TIMER_USB_NOTICE:
            return state->notice_deadline_ms;
        case APP_TIMER_DIM:
            return app_display_idle_deadline(state, UI_DISPLAY_DIM);
        case APP_TIMER_OFF:
            return app_display_idle_deadline(state, UI_DISPLAY_OFF);
        case APP_TIMER_DISPLAY_RETRY:
            return state->display_retry_ms;
        case APP_TIMER_COUNT:
            break;
    }

    return APP_NO_DEADLINE;
}

static uint32_t timer_expired(app_state_t* state, app_timer_t timer)
{
    switch (timer) {
        case APP_TIMER_POWER:
            return APP_EFFECT_POLL_POWER;
        case APP_TIMER_FIRST_MEASUREMENT:
            state->first_measurement_deadline_ms = APP_NO_DEADLINE;

            return APP_EFFECT_MODEL;
        case APP_TIMER_DIALOG:
            return close_dialog(state);
        case APP_TIMER_SETTINGS:
            return app_save_settings(state);
        case APP_TIMER_USB_NOTICE:
            return app_display_close_usb_notice(state);
        case APP_TIMER_DIM:
            return app_display_request(state, UI_DISPLAY_DIM);
        case APP_TIMER_OFF:
            return app_display_off(state);
        case APP_TIMER_DISPLAY_RETRY:
            state->display_retry_ms = APP_NO_DEADLINE;

            return APP_EFFECT_DISPLAY;
        case APP_TIMER_COUNT:
            break;
    }

    return APP_EFFECT_NONE;
}

uint32_t app_state_tick(app_state_t* state, int64_t now_ms)
{
    if (state->mode == APP_MODE_SHUTTING_DOWN) {
        return APP_EFFECT_NONE;
    }

    uint32_t effects = APP_EFFECT_NONE;
    for (app_timer_t timer = 0; timer < APP_TIMER_COUNT; timer++) {
        // Earlier timeouts can cancel or replace later deadlines in this pass.
        if (now_ms >= timer_deadline(state, timer)) {
            effects |= timer_expired(state, timer);
        }
    }

    return effects;
}

uint32_t app_state_cancel_dialog(app_state_t* state)
{
    if (state->screen != UI_SCREEN_SHUTDOWN) {
        return APP_EFFECT_NONE;
    }

    return close_dialog(state);
}

uint32_t app_state_input_lost(app_state_t* state)
{
    state->held_buttons = 0;
    state->wake_buttons = 0;

    return app_state_cancel_dialog(state);
}

int64_t app_state_next_deadline(const app_state_t* state)
{
    if (state->mode == APP_MODE_SHUTTING_DOWN) {
        return APP_NO_DEADLINE;
    }

    int64_t deadline = APP_NO_DEADLINE;
    for (app_timer_t timer = 0; timer < APP_TIMER_COUNT; timer++) {
        const int64_t candidate = timer_deadline(state, timer);
        if (candidate < deadline) {
            deadline = candidate;
        }
    }

    return deadline;
}
