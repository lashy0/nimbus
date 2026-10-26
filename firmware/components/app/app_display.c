#include "app_internal.h"

// A request that failed before is sent again even when it is the same.
uint32_t app_display_request(app_state_t* state, ui_display_state_t display)
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
uint32_t app_display_off(app_state_t* state)
{
    state->screen = state->main_screen;
    state->dialog_deadline_ms = APP_NO_DEADLINE;
    state->notice_deadline_ms = APP_NO_DEADLINE;
    state->usb_notice_woke_display = false;

    return APP_EFFECT_MODEL | app_save_settings(state) | app_display_request(state, UI_DISPLAY_OFF);
}

uint32_t app_display_close_usb_notice(app_state_t* state)
{
    if (state->usb_notice_woke_display) {
        return app_display_off(state);
    }

    state->notice_deadline_ms = APP_NO_DEADLINE;

    return APP_EFFECT_MODEL;
}

// Refresh the power status before drawing the first frame after a button wake.
static uint32_t wake_from_button(app_state_t* state)
{
    state->mode = APP_MODE_ACTIVE;

    return app_display_request(state, UI_DISPLAY_ON) | APP_EFFECT_MODEL | APP_EFFECT_POLL_POWER;
}

uint32_t app_display_button(app_state_t* state, const app_button_event_t* event, bool* consumed)
{
    const uint8_t bit = 1u << event->button;
    *consumed = true;

    // A queue reset can leave gestures and a release without their press. They
    // must not act on a page or cancel the automatic return from a USB wake.
    if (event->gesture != APP_GESTURE_PRESSED && !(state->held_buttons & bit)) {
        return APP_EFFECT_NONE;
    }

    state->activity_ms = event->timestamp_ms;
    state->usb_notice_woke_display = false;

    if (event->gesture == APP_GESTURE_PRESSED) {
        state->held_buttons |= bit;

        // A DIM/OFF wake press only restores brightness; consume its gestures until release.
        if (state->display != UI_DISPLAY_ON) {
            state->wake_buttons |= bit;

            if (state->display == UI_DISPLAY_OFF) {
                return wake_from_button(state);
            }
        }

        return app_display_request(state, UI_DISPLAY_ON);
    }

    if (event->gesture == APP_GESTURE_RELEASED) {
        state->held_buttons &= ~bit;
        state->wake_buttons &= ~bit;

        return APP_EFFECT_NONE;
    }

    if ((state->wake_buttons & bit) || state->display == UI_DISPLAY_OFF) {
        return APP_EFFECT_NONE;
    }

    // A gesture over the USB notice only closes it: the page under it is hidden.
    if (state->notice_deadline_ms != APP_NO_DEADLINE) {
        return app_display_close_usb_notice(state);
    }

    *consumed = false;

    return APP_EFFECT_NONE;
}

uint32_t app_display_power(app_state_t* state, const power_status_t* power, int64_t now_ms)
{
    const power_status_t* shown = &state->power;
    uint32_t effects = APP_EFFECT_NONE;

    // USB found at boot is not a plug-in; the shutdown question stays in front.
    if (shown->source == POWER_SOURCE_BATTERY && power->source == POWER_SOURCE_EXTERNAL &&
        (state->mode == APP_MODE_ACTIVE || state->mode == APP_MODE_MONITORING) && state->screen != UI_SCREEN_SHUTDOWN) {
        state->notice_deadline_ms = now_ms + APP_USB_NOTICE_MS;
        effects |= APP_EFFECT_MODEL;
        if (state->display == UI_DISPLAY_OFF) {
            state->usb_notice_woke_display = true;
            state->activity_ms = now_ms;
            state->mode = APP_MODE_ACTIVE;
            effects |= app_display_request(state, UI_DISPLAY_ON);
        }
    } else if (power->source != POWER_SOURCE_EXTERNAL && state->notice_deadline_ms != APP_NO_DEADLINE) {
        effects |= app_display_close_usb_notice(state);
    }

    return effects;
}

int64_t app_display_idle_deadline(const app_state_t* state, ui_display_state_t display)
{
    if (state->mode != APP_MODE_ACTIVE || state->held_buttons != 0) {
        return APP_NO_DEADLINE;
    }

    if (display == UI_DISPLAY_DIM && state->display == UI_DISPLAY_ON) {
        return state->activity_ms + APP_DIM_AFTER_MS;
    }

    if (display == UI_DISPLAY_OFF && state->display != UI_DISPLAY_OFF) {
        return state->activity_ms + APP_OFF_AFTER_MS;
    }

    return APP_NO_DEADLINE;
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
        return app_begin_shutdown(state, APP_SHUTDOWN_FAULT);
    }

    return APP_EFFECT_NONE;
}
