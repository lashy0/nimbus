#pragma once

#include "app_state.h"

#define PREVIEW_LONG_PRESS_MS 1500        /**< Same hold threshold as board_buttons.c. */
#define PREVIEW_FIRST_MEASUREMENT_MS 2000 /**< Simulated sensor startup delay. */

typedef struct {
    bool pressed;
    bool long_sent;
    int64_t pressed_ms;
} preview_button_t;

typedef struct {
    app_state_t app;
    power_status_t power;
    sensing_snapshot_t measurement;
    preview_button_t buttons[2];
    uint8_t saved_brightness;
    bool sensor_error;
    int64_t measurement_deadline_ms;
} preview_t;

typedef enum {
    PREVIEW_VALUE_INCREASE,       /**< Increase by 1. */
    PREVIEW_VALUE_DECREASE,       /**< Decrease by 1. */
    PREVIEW_VALUE_INCREASE_FAST,  /**< Increase by 10. */
    PREVIEW_VALUE_DECREASE_FAST,  /**< Decrease by 10. */
    PREVIEW_VALUE_NEXT_STATE,     /**< Jump to the lower bound of the next visual state. */
    PREVIEW_VALUE_PREVIOUS_STATE, /**< Jump to the lower bound of the previous visual state. */
    PREVIEW_VALUE_RESET,          /**< Restore this page's initial demo value. */
} preview_value_edit_t;

/**
 * Initializes a desktop session with demo measurements and battery power at monotonic now_ms.
 * Call before other preview functions. All preview calls require serialized access, non-NULL
 * pointers, valid APP_BUTTON_PREV/NEXT buttons and monotonic milliseconds. Returned effects
 * follow preview_tick()'s presentation contract.
 */
uint32_t preview_init(preview_t* preview, int64_t now_ms);

/** Restarts the application, preserving simulated power, sensor conditions and saved brightness. */
uint32_t preview_restart(preview_t* preview, int64_t now_ms);

/**
 * Advances sensor, gesture and application deadlines. Returns MODEL, DISPLAY and SHUTDOWN effects.
 * The caller applies display changes and reports the result with app_state_display_result();
 * preview functions serialize all other application effects.
 */
uint32_t preview_tick(preview_t* preview, int64_t now_ms);

/** Translates a key edge into board-style gestures; repeated presses are ignored. Call serially. */
uint32_t preview_button(preview_t* preview, app_button_t button, bool pressed, int64_t now_ms);

/** Cancels held keys and the open dialog after focus or queued input is lost, without a short press. */
uint32_t preview_input_lost(preview_t* preview);

/** Toggles simulated USB power and delivers a power reading through the application policy. */
uint32_t preview_toggle_usb(preview_t* preview, int64_t now_ms);

/** Toggles a simulated sensor fault, restoring the selected calibration state on recovery. */
uint32_t preview_toggle_error(preview_t* preview, int64_t now_ms);

/** Toggles simulated sensor warmup; a current sensor fault stays visible until recovery. */
uint32_t preview_toggle_calibration(preview_t* preview, int64_t now_ms);

/**
 * Edits the shown measurement page through the application model, clamped to its UI range.
 * Other screens, USB overlays and shutdown ignore edits. Does not wake the display or clear
 * calibration. Values survive navigation and preview_restart(), but not process exit.
 * State jumps stop at the first/last state; humidity includes 100% as a separate font state.
 */
uint32_t preview_edit_value(preview_t* preview, preview_value_edit_t edit, int64_t now_ms);
