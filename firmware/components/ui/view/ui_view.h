#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

#include "buttons_types.h"
#include "power_types.h"
#include "sensing_types.h"

/**
 * @file
 * Screen content. Uses only LVGL, so it also builds for the simulator.
 * Call with exclusive LVGL access.
 */

/**
 * Builds every page and shows the start page in place of the display's default screen.
 *
 * @param brightness Saved backlight percent; one the view cannot set, such as 0, gives its default.
 */
void ui_view_create(uint8_t brightness);

/** Shows a new snapshot. */
void ui_view_update(const sensing_snapshot_t* snapshot);

/** Updates the battery indicator. */
void ui_view_update_power(const power_status_t* status);

/** Handles a button press. */
void ui_view_press(const buttons_event_t* event);

/** Leaves the brightness page and the power-off question; a confirmed power-off stays. */
void ui_view_close_dialogs(void);

/** Backlight percent chosen on the brightness page. */
uint8_t ui_view_brightness(void);

/** True once the user has confirmed powering off; presses are ignored from then on. */
bool ui_view_power_off_confirmed(void);
