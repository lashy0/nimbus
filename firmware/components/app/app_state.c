#include "app_state.h"

#include <math.h>
#include <string.h>

#define IAQ_MAX 500
#define TEMPERATURE_MIN_C -40 // BME680 operating range.
#define TEMPERATURE_MAX_C 85
#define HUMIDITY_MAX 100

typedef enum {
    SENSOR_WAITING, // No output since boot yet; the start screen stays.
    SENSOR_FAULT,   // One error page replaces the measurement pages.
    SENSOR_WORKING,
} sensor_status_t;

static bool is_main_screen(ui_screen_t screen)
{
    return screen == UI_SCREEN_IAQ || screen == UI_SCREEN_TEMP || screen == UI_SCREEN_HUM;
}

// An error ends the wait as well: retries may take far longer than the first output.
static bool first_measurement_arrived(const sensing_snapshot_t* measurement)
{
    return measurement->updated_ms != 0 || measurement->phase == SENSING_PHASE_ERROR;
}

// Working means fresh temperature and humidity; IAQ may still be calibrating.
static sensor_status_t sensor_status(const app_state_t* state)
{
    if (state->first_measurement_deadline_ms != APP_NO_DEADLINE) {
        return SENSOR_WAITING;
    }

    const sensing_snapshot_t* measurement = &state->measurement;
    const bool running = measurement->phase == SENSING_PHASE_WARMUP ||
                         measurement->phase == SENSING_PHASE_CALIBRATING || measurement->phase == SENSING_PHASE_READY;
    const bool values = measurement->temperature_valid && isfinite(measurement->temperature_c) &&
                        measurement->humidity_valid && isfinite(measurement->humidity_percent);

    return running && values && !state->measurement_stale ? SENSOR_WORKING : SENSOR_FAULT;
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

static uint32_t save_settings(app_state_t* state)
{
    if (state->settings_deadline_ms == APP_NO_DEADLINE) {
        return APP_EFFECT_NONE;
    }

    state->settings_deadline_ms = APP_NO_DEADLINE;

    return APP_EFFECT_SAVE_SETTINGS;
}

// The pending settings are saved first; input stays ignored from here on.
static uint32_t begin_shutdown(app_state_t* state, app_shutdown_reason_t reason)
{
    state->mode = APP_MODE_SHUTTING_DOWN;
    state->shutdown_reason = reason;
    state->dialog_deadline_ms = APP_NO_DEADLINE;

    return save_settings(state) | APP_EFFECT_SHUTDOWN;
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

// A request that failed before is sent again even when it is the same.
static uint32_t request_display(app_state_t* state, ui_display_state_t display)
{
    if (state->display == display && state->display_failures == 0) {
        return APP_EFFECT_NONE;
    }

    state->display = display;
    state->display_failures = 0;
    state->display_retry_ms = APP_NO_DEADLINE;

    return APP_EFFECT_DISPLAY;
}

// Monitoring starts with the main screen: open dialogs close and a pending
// brightness is saved, so a wake shows the page the user left.
static uint32_t turn_display_off(app_state_t* state)
{
    state->screen = state->main_screen;
    state->dialog_deadline_ms = APP_NO_DEADLINE;
    state->notice_deadline_ms = APP_NO_DEADLINE;

    return APP_EFFECT_MODEL | save_settings(state) | request_display(state, UI_DISPLAY_OFF);
}

// The battery reading may be a minute old after monitoring.
static uint32_t wake(app_state_t* state)
{
    state->mode = APP_MODE_ACTIVE;

    return request_display(state, UI_DISPLAY_ON) | APP_EFFECT_MODEL | APP_EFFECT_POLL_POWER;
}

static uint32_t on_short(app_state_t* state, app_button_t button, int64_t now_ms)
{
    switch (state->screen) {
        case UI_SCREEN_IAQ:
        case UI_SCREEN_TEMP:
        case UI_SCREEN_HUM:
            // The start screen and the error page stand for all three pages.
            if (sensor_status(state) != SENSOR_WORKING) {
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
                return close_dialog(state) | begin_shutdown(state, APP_SHUTDOWN_USER);
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
    if (is_main_screen(state->screen)) {
        if (button == APP_BUTTON_PREV) {
            return open_shutdown_question(state, now_ms);
        }

        state->screen = UI_SCREEN_BRIGHTNESS;

        return APP_EFFECT_MODEL;
    }

    // Leaving the screen saves a pending change without waiting for the delay.
    if (state->screen == UI_SCREEN_BRIGHTNESS && button == APP_BUTTON_NEXT) {
        state->screen = state->main_screen;

        return APP_EFFECT_MODEL | save_settings(state);
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
    state->measurement_stale = true;
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
    const uint8_t bit = 1u << event->button;

    // A press at a deadline already belongs to what the deadline starts.
    uint32_t effects = app_state_tick(state, now_ms);
    state->activity_ms = now_ms;

    if (event->gesture == APP_GESTURE_PRESSED) {
        state->held_buttons |= bit;

        // The press that wakes a dark display does nothing else until its release.
        if (state->display == UI_DISPLAY_OFF) {
            state->wake_buttons |= bit;

            return effects | wake(state);
        }

        return effects | request_display(state, UI_DISPLAY_ON);
    }

    if (event->gesture == APP_GESTURE_RELEASED) {
        state->held_buttons &= ~bit;
        state->wake_buttons &= ~bit;

        return effects;
    }

    if ((state->wake_buttons & bit) || state->display == UI_DISPLAY_OFF) {
        return effects;
    }

    // A gesture over the USB notice only closes it: the page under it is hidden.
    if (state->notice_deadline_ms != APP_NO_DEADLINE) {
        state->notice_deadline_ms = APP_NO_DEADLINE;

        return effects | APP_EFFECT_MODEL;
    }

    if (event->gesture == APP_GESTURE_SHORT) {
        return effects | on_short(state, event->button, now_ms);
    }

    return effects | on_long(state, event->button, now_ms);
}

uint32_t app_state_measurement(app_state_t* state, const sensing_snapshot_t* measurement, int64_t now_ms)
{
    state->measurement = *measurement;
    state->measurement_stale =
        measurement->updated_ms == 0 || now_ms - measurement->updated_ms >= APP_MEASUREMENT_STALE_MS;
    if (first_measurement_arrived(measurement)) {
        state->first_measurement_deadline_ms = APP_NO_DEADLINE;
    }

    // Reading the battery now saves a wake-up of its own a few seconds later.
    uint32_t effects = APP_EFFECT_MODEL;
    if (state->mode == APP_MODE_MONITORING && now_ms - state->power_polled_ms >= APP_POWER_POLL_EARLY_MS) {
        effects |= APP_EFFECT_POLL_POWER;
    }

    return effects;
}

uint32_t app_state_power(app_state_t* state, const power_status_t* power, int64_t now_ms)
{
    const bool monitoring = state->mode == APP_MODE_MONITORING;
    state->power_polled_ms = now_ms;
    state->power_deadline_ms = now_ms + (monitoring ? APP_POWER_POLL_MONITORING_MS : APP_POWER_POLL_MS);
    if (state->mode == APP_MODE_SHUTTING_DOWN) {
        return APP_EFFECT_NONE;
    }

    const power_status_t* shown = &state->power;
    const bool changed = power->source != shown->source || power->percent_valid != shown->percent_valid ||
                         power->percent != shown->percent;

    // Only a plug-in is announced, not USB found at boot, and only on a lit display;
    // the shutdown question stays in front of it.
    if (shown->source == POWER_SOURCE_BATTERY && power->source == POWER_SOURCE_EXTERNAL &&
        state->mode == APP_MODE_ACTIVE && state->display != UI_DISPLAY_OFF && state->screen != UI_SCREEN_SHUTDOWN) {
        state->notice_deadline_ms = now_ms + APP_USB_NOTICE_MS;
    } else if (power->source != POWER_SOURCE_EXTERNAL) {
        state->notice_deadline_ms = APP_NO_DEADLINE;
    }

    state->power = *power;

    uint32_t effects = changed ? APP_EFFECT_MODEL : APP_EFFECT_NONE;
    if (power->critical) {
        effects |= app_state_cancel_dialog(state) | begin_shutdown(state, APP_SHUTDOWN_CRITICAL);
    }

    return effects;
}

static int64_t stale_deadline(const app_state_t* state)
{
    if (state->measurement_stale || state->measurement.updated_ms == 0) {
        return APP_NO_DEADLINE;
    }

    return state->measurement.updated_ms + APP_MEASUREMENT_STALE_MS;
}

// Idle time counts only in ACTIVE with every button released.
static bool idle_counts(const app_state_t* state)
{
    return state->mode == APP_MODE_ACTIVE && state->held_buttons == 0;
}

static int64_t dim_deadline(const app_state_t* state)
{
    if (!idle_counts(state) || state->display != UI_DISPLAY_ON) {
        return APP_NO_DEADLINE;
    }

    return state->activity_ms + APP_DIM_AFTER_MS;
}

static int64_t off_deadline(const app_state_t* state)
{
    if (!idle_counts(state) || state->display == UI_DISPLAY_OFF) {
        return APP_NO_DEADLINE;
    }

    return state->activity_ms + APP_OFF_AFTER_MS;
}

uint32_t app_state_tick(app_state_t* state, int64_t now_ms)
{
    if (state->mode == APP_MODE_SHUTTING_DOWN) {
        return APP_EFFECT_NONE;
    }

    uint32_t effects = now_ms >= state->power_deadline_ms ? APP_EFFECT_POLL_POWER : APP_EFFECT_NONE;
    if (now_ms >= stale_deadline(state)) {
        state->measurement_stale = true;
        effects |= APP_EFFECT_MODEL;
    }

    if (now_ms >= state->first_measurement_deadline_ms) {
        state->first_measurement_deadline_ms = APP_NO_DEADLINE;
        effects |= APP_EFFECT_MODEL;
    }

    if (state->screen == UI_SCREEN_SHUTDOWN && now_ms >= state->dialog_deadline_ms) {
        effects |= close_dialog(state);
    }

    if (now_ms >= state->settings_deadline_ms) {
        effects |= save_settings(state);
    }

    if (now_ms >= state->notice_deadline_ms) {
        state->notice_deadline_ms = APP_NO_DEADLINE;
        effects |= APP_EFFECT_MODEL;
    }

    if (now_ms >= dim_deadline(state)) {
        effects |= request_display(state, UI_DISPLAY_DIM);
    }

    if (now_ms >= off_deadline(state)) {
        effects |= turn_display_off(state);
    }

    if (now_ms >= state->display_retry_ms) {
        state->display_retry_ms = APP_NO_DEADLINE;
        effects |= APP_EFFECT_DISPLAY;
    }

    return effects;
}

uint8_t app_state_backlight(const app_state_t* state)
{
    switch (state->display) {
        case UI_DISPLAY_ON:
            return state->brightness_percent;
        case UI_DISPLAY_DIM:
            return state->brightness_percent < APP_DIM_BRIGHTNESS_MAX ? state->brightness_percent
                                                                      : APP_DIM_BRIGHTNESS_MAX;
        case UI_DISPLAY_OFF:
            break;
    }

    return 0;
}

uint32_t app_state_display_result(app_state_t* state, bool reached, int64_t now_ms)
{
    if (reached) {
        state->display_failures = 0;
        state->display_retry_ms = APP_NO_DEADLINE;
        if (state->display == UI_DISPLAY_OFF && state->mode == APP_MODE_ACTIVE) {
            state->mode = APP_MODE_MONITORING;
        }

        return APP_EFFECT_NONE;
    }

    state->display_failures++;
    if (state->display_failures <= APP_DISPLAY_RETRIES) {
        state->display_retry_ms = now_ms + APP_DISPLAY_RETRY_MS;

        return APP_EFFECT_NONE;
    }

    // A display that cannot go dark keeps the chip out of light sleep for good;
    // a failed ON waits for the next button event instead.
    state->display_retry_ms = APP_NO_DEADLINE;
    if (state->display == UI_DISPLAY_OFF && state->mode == APP_MODE_ACTIVE) {
        return begin_shutdown(state, APP_SHUTDOWN_FAULT);
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

uint32_t app_state_input_lost(app_state_t* state)
{
    state->held_buttons = 0;

    return app_state_cancel_dialog(state);
}

int64_t app_state_next_deadline(const app_state_t* state)
{
    if (state->mode == APP_MODE_SHUTTING_DOWN) {
        return APP_NO_DEADLINE;
    }

    int64_t deadline = stale_deadline(state);
    if (dim_deadline(state) < deadline) {
        deadline = dim_deadline(state);
    }

    if (off_deadline(state) < deadline) {
        deadline = off_deadline(state);
    }

    if (state->display_retry_ms < deadline) {
        deadline = state->display_retry_ms;
    }

    if (state->power_deadline_ms < deadline) {
        deadline = state->power_deadline_ms;
    }

    if (state->settings_deadline_ms < deadline) {
        deadline = state->settings_deadline_ms;
    }

    if (state->notice_deadline_ms < deadline) {
        deadline = state->notice_deadline_ms;
    }

    if (state->first_measurement_deadline_ms < deadline) {
        deadline = state->first_measurement_deadline_ms;
    }

    if (state->screen == UI_SCREEN_SHUTDOWN && state->dialog_deadline_ms < deadline) {
        deadline = state->dialog_deadline_ms;
    }

    return deadline;
}

static long round_clamped(float value, long min, long max)
{
    const long rounded = lroundf(value);
    if (rounded < min) {
        return min;
    }

    return rounded > max ? max : rounded;
}

static ui_screen_t shown_screen(const app_state_t* state)
{
    if (!is_main_screen(state->screen)) {
        return state->screen;
    }

    switch (sensor_status(state)) {
        case SENSOR_WAITING:
            return UI_SCREEN_START;
        case SENSOR_FAULT:
            return UI_SCREEN_SENSOR_ERROR;
        case SENSOR_WORKING:
            break;
    }

    return state->screen;
}

void app_state_model(const app_state_t* state, ui_model_t* model)
{
    const sensing_snapshot_t* measurement = &state->measurement;
    memset(model, 0, sizeof(*model));

    model->screen = shown_screen(state);
    model->usb_notice = state->notice_deadline_ms != APP_NO_DEADLINE;
    model->external_power = state->power.source == POWER_SOURCE_EXTERNAL;
    model->battery_valid = state->power.percent_valid;
    model->battery_percent = state->power.percent;
    model->brightness_percent = state->brightness_percent;
    model->shutdown_yes_selected = state->shutdown_yes_selected;

    if (sensor_status(state) != SENSOR_WORKING) {
        return;
    }

    model->iaq_calibrating =
        measurement->phase == SENSING_PHASE_WARMUP || !measurement->iaq_valid || !isfinite(measurement->static_iaq);
    if (!model->iaq_calibrating) {
        model->iaq = (uint16_t)round_clamped(measurement->static_iaq, 0, IAQ_MAX);
    }

    model->temperature_c = (int16_t)round_clamped(measurement->temperature_c, TEMPERATURE_MIN_C, TEMPERATURE_MAX_C);
    model->humidity_percent = (uint8_t)round_clamped(measurement->humidity_percent, 0, HUMIDITY_MAX);
}
