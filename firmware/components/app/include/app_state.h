#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "app_events.h"
#include "sensing_types.h"
#include "ui_model.h"

#define APP_BRIGHTNESS_MIN 5
#define APP_BRIGHTNESS_MAX 100
#define APP_BRIGHTNESS_STEP 5
#define APP_BRIGHTNESS_DEFAULT 60
#define APP_SETTINGS_SAVE_DELAY_MS 2000 // After the last change, so holding a button writes once.
#define APP_DIALOG_TIMEOUT_MS 15000
#define APP_MEASUREMENT_STALE_MS 10000 // LP outputs arrive every 3 s.

#define APP_NO_DEADLINE INT64_MAX

typedef enum {
    APP_MODE_STARTING, // Input is ignored until startup finishes.
    APP_MODE_ACTIVE,
} app_mode_t;

// What the caller has to carry out after a transition.
typedef enum {
    APP_EFFECT_NONE = 0,
    APP_EFFECT_MODEL = 1 << 0,         // The screen model changed.
    APP_EFFECT_BRIGHTNESS = 1 << 1,    // The backlight must follow brightness_percent.
    APP_EFFECT_SHUTDOWN = 1 << 2,      // The user confirmed power-off.
    APP_EFFECT_SAVE_SETTINGS = 1 << 3, // brightness_percent must be saved.
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
    bool measurement_stale; // No new outputs for APP_MEASUREMENT_STALE_MS.
} app_state_t;

// Whether a saved brightness is one the brightness screen can produce.
bool app_brightness_valid(uint8_t percent);

// Times are monotonic milliseconds, the clock of sensing_snapshot_t.updated_ms.
// Functions return a mask of app_effect_t.
void app_state_init(app_state_t* state, uint8_t brightness_percent);
uint32_t app_state_activate(app_state_t* state);
uint32_t app_state_button(app_state_t* state, const app_button_event_t* event, int64_t now_ms);
uint32_t app_state_measurement(app_state_t* state, const sensing_snapshot_t* measurement, int64_t now_ms);
uint32_t app_state_tick(app_state_t* state, int64_t now_ms);

// Closes an open dialog without acting on it, e.g. after input was lost.
uint32_t app_state_cancel_dialog(app_state_t* state);

// Earliest time app_state_tick() has work, or APP_NO_DEADLINE.
int64_t app_state_next_deadline(const app_state_t* state);

void app_state_model(const app_state_t* state, ui_model_t* model);
