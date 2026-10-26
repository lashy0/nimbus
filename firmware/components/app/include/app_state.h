#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "app_events.h"
#include "power_types.h"
#include "sensing_types.h"
#include "ui_model.h"

#define APP_BRIGHTNESS_MIN 5
#define APP_BRIGHTNESS_MAX 100
#define APP_BRIGHTNESS_STEP 5
#define APP_BRIGHTNESS_DEFAULT 60
#define APP_SETTINGS_SAVE_DELAY_MS 2000 // After the last change, so holding a button writes once.
#define APP_DIALOG_TIMEOUT_MS 15000
#define APP_FIRST_MEASUREMENT_MS 10000 // The start screen waits this long for the first output.
#define APP_POWER_POLL_MS 2000
#define APP_POWER_POLL_MONITORING_MS 60000
#define APP_POWER_POLL_EARLY_MS 55000 // In monitoring a sensor wake-up this late reads the battery too.
#define APP_USB_NOTICE_MS 3000
#define APP_DIM_AFTER_MS 30000 // Without button events; a held button keeps the display on.
#define APP_OFF_AFTER_MS 60000
#define APP_DIM_BRIGHTNESS_MAX 10
#define APP_DISPLAY_RETRY_MS 1000
#define APP_DISPLAY_RETRIES 3 // After the first failed request.

#define APP_NO_DEADLINE INT64_MAX

typedef enum {
    APP_MODE_STARTING, // Input is ignored until startup finishes.
    APP_MODE_ACTIVE,
    APP_MODE_MONITORING,    // The display is confirmed OFF; measurements go on.
    APP_MODE_SHUTTING_DOWN, // Power-off under way; input is ignored for good.
} app_mode_t;

typedef enum {
    APP_SHUTDOWN_USER,     // Confirmed on screen.
    APP_SHUTDOWN_CRITICAL, // Confirmed battery discharge.
    APP_SHUTDOWN_FAULT,    // The display cannot be turned off, so the chip cannot sleep.
} app_shutdown_reason_t;

// What the caller has to carry out after a transition.
typedef enum {
    APP_EFFECT_NONE = 0,
    APP_EFFECT_MODEL = 1 << 0,         // The screen model changed.
    APP_EFFECT_DISPLAY = 1 << 1,       // Request display and app_state_backlight(), then report the result.
    APP_EFFECT_SHUTDOWN = 1 << 2,      // Power off: confirmed by the user or a flat battery.
    APP_EFFECT_SAVE_SETTINGS = 1 << 3, // brightness_percent must be saved.
    APP_EFFECT_POLL_POWER = 1 << 4,    // Read the battery and pass it to app_state_power().
} app_effect_t;

typedef struct {
    app_mode_t mode;
    ui_screen_t screen;
    ui_screen_t main_screen; // IAQ, TEMP or HUM to return to from a dialog.
    uint8_t brightness_percent;
    int64_t settings_deadline_ms; // Pending save of changed settings, or APP_NO_DEADLINE.
    bool shutdown_yes_selected;
    int64_t dialog_deadline_ms; // Auto-cancel of the shutdown question.
    sensing_snapshot_t measurement;
    int64_t first_measurement_deadline_ms; // End of the start screen's wait, or APP_NO_DEADLINE.
    power_status_t power;
    int64_t power_deadline_ms;
    int64_t power_polled_ms;
    int64_t notice_deadline_ms; // End of the USB notice, or APP_NO_DEADLINE when none is shown.
    app_shutdown_reason_t shutdown_reason;
    ui_display_state_t display; // Requested from the UI; MONITORING follows a confirmed OFF.
    uint8_t display_failures;   // Failed attempts at the current request.
    int64_t display_retry_ms;   // Next attempt after a failure, or APP_NO_DEADLINE.
    int64_t activity_ms;        // Last button event; dimming and OFF count from it.
    uint8_t held_buttons;       // Bit per app_button_t, pressed and not yet released.
    uint8_t wake_buttons;       // Presses that woke the display, consumed until release.
} app_state_t;

// Whether a saved brightness is one the brightness screen can produce.
bool app_brightness_valid(uint8_t percent);

// Times are monotonic milliseconds since boot.
// Functions return a mask of app_effect_t.
void app_state_init(app_state_t* state, uint8_t brightness_percent);
uint32_t app_state_activate(app_state_t* state, int64_t now_ms);

// Acts at event->timestamp_ms, so a queued press is judged by when it happened.
uint32_t app_state_button(app_state_t* state, const app_button_event_t* event);
uint32_t app_state_measurement(app_state_t* state, const sensing_snapshot_t* measurement, int64_t now_ms);
uint32_t app_state_power(app_state_t* state, const power_status_t* power, int64_t now_ms);
uint32_t app_state_tick(app_state_t* state, int64_t now_ms);

// Backlight percent for the requested display state.
uint8_t app_state_backlight(const app_state_t* state);

// Reports whether the UI reached state->display after APP_EFFECT_DISPLAY. A failure
// is retried APP_DISPLAY_RETRIES times; a failed OFF keeps the device ACTIVE, and
// an OFF that still fails after that powers it off.
uint32_t app_state_display_result(app_state_t* state, bool reached, int64_t now_ms);

// Closes an open dialog without acting on it.
uint32_t app_state_cancel_dialog(app_state_t* state);

// Button events were dropped: a lost release must not keep the display on for good.
uint32_t app_state_input_lost(app_state_t* state);

// Earliest time app_state_tick() has work, or APP_NO_DEADLINE.
int64_t app_state_next_deadline(const app_state_t* state);

void app_state_model(const app_state_t* state, ui_model_t* model);
