#include "preview.h"

#include <string.h>

static const sensing_snapshot_t DEMO_MEASUREMENT = {
    .phase = SENSING_PHASE_READY,
    .iaq = 42,
    .iaq_accuracy = 3,
    .temperature_c = 23,
    .humidity_percent = 45,
    .pressure_pa = 101325,
};

static uint32_t deliver_measurement(preview_t* preview)
{
    sensing_snapshot_t measurement = preview->measurement;
    if (preview->sensor_error) {
        measurement.phase = SENSING_PHASE_ERROR;
    }

    return app_state_measurement(&preview->app, &measurement);
}

static uint32_t resolve_effects(preview_t* preview, uint32_t effects, int64_t now_ms)
{
    if (effects & APP_EFFECT_POLL_POWER) {
        effects &= ~APP_EFFECT_POLL_POWER;
        effects |= app_state_power(&preview->app, &preview->power, now_ms);
    }

    if (effects & APP_EFFECT_SAVE_SETTINGS) {
        // Desktop settings live for this process, including simulated restarts.
        preview->saved_brightness = preview->app.brightness_percent;
        effects &= ~APP_EFFECT_SAVE_SETTINGS;
    }

    return effects;
}

static uint32_t send_gesture(preview_t* preview, app_button_t button, app_gesture_t gesture, int64_t now_ms)
{
    const app_button_event_t event = {
        .button = button,
        .gesture = gesture,
        .timestamp_ms = now_ms,
    };

    return resolve_effects(preview, app_state_button(&preview->app, &event), now_ms);
}

uint32_t preview_restart(preview_t* preview, int64_t now_ms)
{
    memset(preview->buttons, 0, sizeof(preview->buttons));
    app_state_init(&preview->app, preview->saved_brightness);
    preview->measurement_deadline_ms = now_ms + PREVIEW_FIRST_MEASUREMENT_MS;

    return resolve_effects(preview, app_state_activate(&preview->app, now_ms), now_ms) | APP_EFFECT_MODEL |
           APP_EFFECT_DISPLAY;
}

uint32_t preview_init(preview_t* preview, int64_t now_ms)
{
    *preview = (preview_t){
        .power = {.source = POWER_SOURCE_BATTERY, .battery_mv = 4000, .percent_valid = true, .percent = 85},
        .measurement = DEMO_MEASUREMENT,
        .saved_brightness = APP_BRIGHTNESS_DEFAULT,
    };

    return preview_restart(preview, now_ms);
}

uint32_t preview_tick(preview_t* preview, int64_t now_ms)
{
    uint32_t effects = APP_EFFECT_NONE;
    // Replay due input at its recognition time, including a release at the hold threshold.
    for (;;) {
        int64_t next_ms = preview->measurement_deadline_ms;
        int next_button = -1;
        for (int i = 0; i < 2; i++) {
            const preview_button_t* button = &preview->buttons[i];
            const int64_t held_ms = button->pressed_ms + PREVIEW_LONG_PRESS_MS;
            if (button->pressed && !button->long_sent && held_ms < next_ms) {
                next_ms = held_ms;
                next_button = i;
            }
        }

        if (next_ms == APP_NO_DEADLINE || next_ms > now_ms) {
            break;
        }

        effects |= resolve_effects(preview, app_state_tick(&preview->app, next_ms), next_ms);
        if (next_button >= 0) {
            preview->buttons[next_button].long_sent = true;
            effects |= send_gesture(preview, (app_button_t)next_button, APP_GESTURE_LONG, next_ms);
        } else {
            preview->measurement_deadline_ms = APP_NO_DEADLINE;
            effects |= deliver_measurement(preview);
        }
    }

    return effects | resolve_effects(preview, app_state_tick(&preview->app, now_ms), now_ms);
}

uint32_t preview_button(preview_t* preview, app_button_t button, bool pressed, int64_t now_ms)
{
    uint32_t effects = preview_tick(preview, now_ms);
    preview_button_t* slot = &preview->buttons[button];
    if (pressed == slot->pressed) {
        return effects;
    }

    slot->pressed = pressed;
    if (pressed) {
        slot->pressed_ms = now_ms;
        slot->long_sent = false;

        return effects | send_gesture(preview, button, APP_GESTURE_PRESSED, now_ms);
    }

    if (!slot->long_sent) {
        effects |= send_gesture(preview, button, APP_GESTURE_SHORT, now_ms);
    }

    return effects | send_gesture(preview, button, APP_GESTURE_RELEASED, now_ms);
}

uint32_t preview_input_lost(preview_t* preview)
{
    memset(preview->buttons, 0, sizeof(preview->buttons));

    return app_state_input_lost(&preview->app);
}

uint32_t preview_toggle_usb(preview_t* preview, int64_t now_ms)
{
    uint32_t effects = preview_tick(preview, now_ms);
    const bool external = preview->power.source != POWER_SOURCE_EXTERNAL;
    preview->power.source = external ? POWER_SOURCE_EXTERNAL : POWER_SOURCE_BATTERY;
    preview->power.battery_mv = external ? 4500 : 4000;
    preview->power.percent_valid = !external;

    return effects | app_state_power(&preview->app, &preview->power, now_ms);
}

uint32_t preview_toggle_error(preview_t* preview, int64_t now_ms)
{
    uint32_t effects = preview_tick(preview, now_ms);
    preview->sensor_error = !preview->sensor_error;
    preview->measurement_deadline_ms = APP_NO_DEADLINE;

    return effects | deliver_measurement(preview);
}

uint32_t preview_toggle_calibration(preview_t* preview, int64_t now_ms)
{
    uint32_t effects = preview_tick(preview, now_ms);
    const bool calibrating = preview->measurement.phase != SENSING_PHASE_WARMUP;
    preview->measurement.phase = calibrating ? SENSING_PHASE_WARMUP : SENSING_PHASE_READY;
    preview->measurement.iaq_accuracy = calibrating ? 0 : 3;

    if (preview->measurement_deadline_ms != APP_NO_DEADLINE) {
        return effects;
    }

    return effects | deliver_measurement(preview);
}

static int edited_value(int value, preview_value_edit_t edit, int initial, const int* states, size_t count, int maximum)
{
    switch (edit) {
        case PREVIEW_VALUE_INCREASE:
            value++;
            break;
        case PREVIEW_VALUE_DECREASE:
            value--;
            break;
        case PREVIEW_VALUE_INCREASE_FAST:
            value += 10;
            break;
        case PREVIEW_VALUE_DECREASE_FAST:
            value -= 10;
            break;
        case PREVIEW_VALUE_RESET:
            return initial;
        case PREVIEW_VALUE_NEXT_STATE:
        case PREVIEW_VALUE_PREVIOUS_STATE: {
            size_t state = 0;
            while (state + 1 < count && value >= states[state + 1]) {
                state++;
            }

            if (edit == PREVIEW_VALUE_NEXT_STATE && state + 1 < count) {
                return states[state + 1];
            }

            if (edit == PREVIEW_VALUE_PREVIOUS_STATE && state > 0) {
                return states[state - 1];
            }

            return value;
        }
    }

    if (value < states[0]) {
        return states[0];
    }

    return value > maximum ? maximum : value;
}

uint32_t preview_edit_value(preview_t* preview, preview_value_edit_t edit, int64_t now_ms)
{
    uint32_t effects = preview_tick(preview, now_ms);
    ui_model_t model;
    app_state_model(&preview->app, &model);
    if (preview->app.mode == APP_MODE_SHUTTING_DOWN || model.usb_notice) {
        return effects;
    }

    // These are preview presets matching ui_view.c, including humidity's smaller 100% font.
    static const int iaq_states[] = {0, 51, 101, 201, 301, 401};
    static const int temperature_states[] = {-40, 0, 26};
    static const int humidity_states[] = {0, 30, 70, 100};
    float* value;
    int initial;
    int maximum;
    const int* states;
    size_t count;
    switch (model.screen) {
        case UI_SCREEN_IAQ:
            value = &preview->measurement.iaq;
            initial = (int)DEMO_MEASUREMENT.iaq;
            maximum = 500;
            states = iaq_states;
            count = sizeof(iaq_states) / sizeof(iaq_states[0]);
            break;
        case UI_SCREEN_TEMP:
            value = &preview->measurement.temperature_c;
            initial = (int)DEMO_MEASUREMENT.temperature_c;
            maximum = 85;
            states = temperature_states;
            count = sizeof(temperature_states) / sizeof(temperature_states[0]);
            break;
        case UI_SCREEN_HUM:
            value = &preview->measurement.humidity_percent;
            initial = (int)DEMO_MEASUREMENT.humidity_percent;
            maximum = 100;
            states = humidity_states;
            count = sizeof(humidity_states) / sizeof(humidity_states[0]);
            break;
        default:
            return effects;
    }

    *value = (float)edited_value((int)*value, edit, initial, states, count, maximum);

    return effects | deliver_measurement(preview);
}
