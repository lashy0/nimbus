#include "app_state.h"

#include <math.h>
#include <string.h>

#define IAQ_MAX 500
#define TEMPERATURE_MIN_C -40 // BME680 operating range.
#define TEMPERATURE_MAX_C 85
#define HUMIDITY_MAX 100

static bool is_main_screen(ui_screen_t screen)
{
    return screen == UI_SCREEN_IAQ || screen == UI_SCREEN_TEMP || screen == UI_SCREEN_HUM;
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

static uint32_t change_brightness(app_state_t* state, int delta)
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

    return APP_EFFECT_MODEL | APP_EFFECT_BRIGHTNESS;
}

static uint32_t on_short(app_state_t* state, app_button_t button)
{
    switch (state->screen) {
        case UI_SCREEN_IAQ:
        case UI_SCREEN_TEMP:
        case UI_SCREEN_HUM:
            state->screen = step_main_screen(state->screen, button);
            state->main_screen = state->screen;

            return APP_EFFECT_MODEL;

        case UI_SCREEN_BRIGHTNESS:
            return change_brightness(state, button == APP_BUTTON_NEXT ? APP_BRIGHTNESS_STEP : -APP_BRIGHTNESS_STEP);

        case UI_SCREEN_SHUTDOWN:
            // A press on the highlighted answer chooses it; otherwise it moves the highlight.
            if (button == APP_BUTTON_PREV && state->shutdown_yes_selected) {
                return close_dialog(state) | APP_EFFECT_SHUTDOWN;
            }

            if (button == APP_BUTTON_NEXT && !state->shutdown_yes_selected) {
                return close_dialog(state);
            }

            state->shutdown_yes_selected = button == APP_BUTTON_PREV;

            return APP_EFFECT_MODEL;

        case UI_SCREEN_START:
            break;
    }

    return APP_EFFECT_NONE;
}

static uint32_t on_long(app_state_t* state, app_button_t button, int64_t now_ms)
{
    if (is_main_screen(state->screen)) {
        if (button == APP_BUTTON_PREV) {
            return open_shutdown_question(state, now_ms);
        }

        state->screen = UI_SCREEN_BRIGHTNESS;

        return APP_EFFECT_MODEL;
    }

    // Brightness is not persisted yet: there is no storage component.
    if (state->screen == UI_SCREEN_BRIGHTNESS && button == APP_BUTTON_NEXT) {
        state->screen = state->main_screen;

        return APP_EFFECT_MODEL;
    }

    return APP_EFFECT_NONE;
}

void app_state_init(app_state_t* state, uint8_t brightness_percent)
{
    memset(state, 0, sizeof(*state));
    state->mode = APP_MODE_STARTING;
    state->screen = UI_SCREEN_START;
    state->main_screen = UI_SCREEN_IAQ;
    state->brightness_percent = brightness_percent;
    state->dialog_deadline_ms = APP_NO_DEADLINE;
}

uint32_t app_state_activate(app_state_t* state)
{
    if (state->mode != APP_MODE_STARTING) {
        return APP_EFFECT_NONE;
    }

    state->mode = APP_MODE_ACTIVE;
    state->screen = state->main_screen;

    return APP_EFFECT_MODEL;
}

uint32_t app_state_button(app_state_t* state, const app_button_event_t* event, int64_t now_ms)
{
    if (state->mode != APP_MODE_ACTIVE) {
        return APP_EFFECT_NONE;
    }

    // A press at the dialog deadline already belongs to the main screen.
    const uint32_t effects = app_state_tick(state, now_ms);

    switch (event->gesture) {
        case APP_GESTURE_SHORT:
            return effects | on_short(state, event->button);
        case APP_GESTURE_LONG:
            return effects | on_long(state, event->button, now_ms);
        case APP_GESTURE_PRESSED:
        case APP_GESTURE_RELEASED:
            break;
    }

    return effects;
}

uint32_t app_state_readings(app_state_t* state, const app_readings_t* readings)
{
    state->readings = *readings;

    return APP_EFFECT_MODEL;
}

uint32_t app_state_tick(app_state_t* state, int64_t now_ms)
{
    if (state->screen == UI_SCREEN_SHUTDOWN && now_ms >= state->dialog_deadline_ms) {
        return close_dialog(state);
    }

    return APP_EFFECT_NONE;
}

uint32_t app_state_cancel_dialog(app_state_t* state)
{
    if (state->screen != UI_SCREEN_SHUTDOWN) {
        return APP_EFFECT_NONE;
    }

    return close_dialog(state);
}

int64_t app_state_next_deadline(const app_state_t* state)
{
    return state->screen == UI_SCREEN_SHUTDOWN ? state->dialog_deadline_ms : APP_NO_DEADLINE;
}

static long round_clamped(float value, long min, long max)
{
    const long rounded = lroundf(value);
    if (rounded < min) {
        return min;
    }

    return rounded > max ? max : rounded;
}

void app_state_model(const app_state_t* state, ui_model_t* model)
{
    const app_readings_t* readings = &state->readings;
    memset(model, 0, sizeof(*model));

    model->screen = state->screen;
    model->brightness_percent = state->brightness_percent;
    model->shutdown_yes_selected = state->shutdown_yes_selected;

    model->battery_valid = readings->battery_valid && readings->battery_percent <= 100;
    model->battery_percent = readings->battery_percent;
    model->charging = readings->charging;

    if (readings->iaq_valid && isfinite(readings->iaq)) {
        model->iaq_state = readings->iaq_accuracy == 0 ? UI_IAQ_CALIBRATING : UI_IAQ_VALID;
        model->iaq = (uint16_t)round_clamped(readings->iaq, 0, IAQ_MAX);
    }

    model->temperature_valid = readings->temperature_valid && isfinite(readings->temperature_c);
    if (model->temperature_valid) {
        model->temperature_c = (int16_t)round_clamped(readings->temperature_c, TEMPERATURE_MIN_C, TEMPERATURE_MAX_C);
    }

    model->humidity_valid = readings->humidity_valid && isfinite(readings->humidity_percent);
    if (model->humidity_valid) {
        model->humidity_percent = (uint8_t)round_clamped(readings->humidity_percent, 0, HUMIDITY_MAX);
    }
}
