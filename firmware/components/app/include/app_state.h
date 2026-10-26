#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "app_events.h"
#include "power_types.h"
#include "sensing_types.h"
#include "ui_model.h"

/**
 * @file
 * Pure application transitions: no hardware calls, allocation or blocking.
 * Callers own the state, serialize access and pass valid, non-NULL pointers.
 * Times are monotonic milliseconds since boot. Functions returning uint32_t
 * return an app_effect_t mask; the caller must carry out the requested effects.
 */

#define APP_BRIGHTNESS_MIN 5
#define APP_BRIGHTNESS_MAX 100
#define APP_BRIGHTNESS_STEP 5
#define APP_BRIGHTNESS_DEFAULT 60
#define APP_SETTINGS_SAVE_DELAY_MS 2000 /**< After the last change, so holding a button writes once. */
#define APP_DIALOG_TIMEOUT_MS 15000
#define APP_FIRST_MEASUREMENT_MS 10000 /**< The start screen waits this long for the first output. */
#define APP_POWER_POLL_MS 2000         /**< Also while dark, so USB can wake the display promptly. */
#define APP_USB_NOTICE_MS 3000
#define APP_DIM_AFTER_MS 30000 /**< Without button events; a held button keeps the display on. */
#define APP_OFF_AFTER_MS 60000
#define APP_DIM_BRIGHTNESS_MAX 10
#define APP_DISPLAY_RETRY_MS 1000
#define APP_DISPLAY_RETRIES 3 /**< After the first failed request. */

#define APP_NO_DEADLINE INT64_MAX

typedef enum {
    APP_MODE_STARTING, /**< Input is ignored until startup finishes. */
    APP_MODE_ACTIVE,
    APP_MODE_MONITORING,    /**< The display is confirmed OFF; measurements go on. */
    APP_MODE_SHUTTING_DOWN, /**< Power-off under way; input is ignored for good. */
} app_mode_t;

typedef enum {
    APP_SHUTDOWN_USER,     /**< Confirmed on screen. */
    APP_SHUTDOWN_CRITICAL, /**< Confirmed battery discharge. */
    APP_SHUTDOWN_FAULT,    /**< The display cannot be turned off, so the chip cannot sleep. */
} app_shutdown_reason_t;

/** Effects the caller must carry out after a transition. */
typedef enum {
    APP_EFFECT_NONE = 0,
    APP_EFFECT_MODEL = 1 << 0,         /**< The screen model changed. */
    APP_EFFECT_DISPLAY = 1 << 1,       /**< Request display and app_state_backlight(), then report the result. */
    APP_EFFECT_SHUTDOWN = 1 << 2,      /**< Power off: confirmed by the user or a flat battery. */
    APP_EFFECT_SAVE_SETTINGS = 1 << 3, /**< brightness_percent must be saved. */
    APP_EFFECT_POLL_POWER = 1 << 4,    /**< Read the battery and pass it to app_state_power(). */
} app_effect_t;

typedef struct {
    app_mode_t mode;
    ui_screen_t screen;
    ui_screen_t main_screen; /**< IAQ, TEMP or HUM to return to from a dialog. */
    uint8_t brightness_percent;
    int64_t settings_deadline_ms; /**< Pending save of changed settings, or APP_NO_DEADLINE. */
    bool shutdown_yes_selected;
    int64_t dialog_deadline_ms; /**< Auto-cancel of the shutdown question. */
    sensing_snapshot_t measurement;
    int64_t first_measurement_deadline_ms; /**< End of the start screen's wait, or APP_NO_DEADLINE. */
    power_status_t power;
    int64_t power_deadline_ms;
    int64_t notice_deadline_ms;   /**< End of the USB notice, or APP_NO_DEADLINE when none is shown. */
    bool usb_notice_woke_display; /**< Return to OFF after the notice unless a button was used. */
    app_shutdown_reason_t shutdown_reason;
    ui_display_state_t display; /**< Requested from the UI; MONITORING follows a confirmed OFF. */
    uint8_t display_failures;   /**< Failed attempts at the current request. */
    int64_t display_retry_ms;   /**< Next attempt after a failure, or APP_NO_DEADLINE. */
    int64_t activity_ms;        /**< Last button event; dimming and OFF count from it. */
    uint8_t held_buttons;       /**< Bit per app_button_t, pressed and not yet released. */
    uint8_t wake_buttons;       /**< Presses that restored brightness from DIM/OFF, consumed until release. */
} app_state_t;

/** Returns whether percent is in 5..100% and aligned to APP_BRIGHTNESS_STEP. */
bool app_brightness_valid(uint8_t percent);

/** Initializes the start screen; brightness_percent must already pass app_brightness_valid(). */
void app_state_init(app_state_t* state, uint8_t brightness_percent);

/** Leaves STARTING and requests the first power poll; returns no effects if already activated. */
uint32_t app_state_activate(app_state_t* state, int64_t now_ms);

/**
 * Handles a button event at event->timestamp_ms, including deadlines at that time.
 * A press in DIM/OFF restores brightness without acting on the current page;
 * SHORT/LONG gestures from that press are consumed until its release.
 * The event is borrowed only for the call; no hardware action is performed here.
 */
uint32_t app_state_button(app_state_t* state, const app_button_event_t* event);

/** Copies the sensing snapshot and requests a model update; values must match its phase. */
uint32_t app_state_measurement(app_state_t* state, const sensing_snapshot_t* measurement);

/** Copies power status, schedules the next poll and requests shutdown for confirmed critical voltage. */
uint32_t app_state_power(app_state_t* state, const power_status_t* power, int64_t now_ms);

/** Handles deadlines reached at now_ms; the caller executes and reports requested effects. */
uint32_t app_state_tick(app_state_t* state, int64_t now_ms);

/** Returns requested backlight brightness in 0..100%; OFF returns 0, DIM applies its cap. */
uint8_t app_state_backlight(const app_state_t* state);

/**
 * Reports whether the UI reached state->display after APP_EFFECT_DISPLAY.
 * A failure is retried APP_DISPLAY_RETRIES times; a failed OFF keeps the device
 * ACTIVE, and an OFF still failing after the retries requests fault shutdown.
 */
uint32_t app_state_display_result(app_state_t* state, bool reached, int64_t now_ms);

/** Closes an open shutdown dialog without confirming shutdown. */
uint32_t app_state_cancel_dialog(app_state_t* state);

/**
 * Clears held/wake tracking and cancels the dialog after button events were lost.
 * Subsequent gestures without a new press are ignored.
 */
uint32_t app_state_input_lost(app_state_t* state);

/** Returns the earliest monotonic deadline in milliseconds, or APP_NO_DEADLINE. */
int64_t app_state_next_deadline(const app_state_t* state);

/** Fills caller-owned model with rounded, clamped values; does not modify state or retain pointers. */
void app_state_model(const app_state_t* state, ui_model_t* model);
