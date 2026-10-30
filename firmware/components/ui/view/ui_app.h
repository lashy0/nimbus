#pragma once

#include <stdint.h>

#include "buttons_types.h"
#include "power_types.h"

/**
 * @file
 * The view and the backlight together. Without presses the screen dims after
 * 30 s and goes dark after 60 s; the next press only brings it back. The USB
 * notice lights a dim or dark screen, which goes back once the notice closes
 * unless a button is pressed meanwhile. Call with exclusive LVGL access.
 */

/**
 * Builds the view and lights the screen.
 *
 * @param brightness Saved backlight percent; one the view cannot set, such as 0, gives its default.
 */
void ui_app_create(uint8_t brightness);

void ui_app_press(const buttons_event_t* event);

/** Updates the view and lights the screen for the USB notice. */
void ui_app_update_power(const power_status_t* status);

/** Saves a brightness change now instead of 2 s after the last one. */
void ui_app_save_pending(void);
