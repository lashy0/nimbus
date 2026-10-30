#pragma once

#include <stdint.h>

#include "lvgl.h"

#include "buttons_types.h"
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

/** Handles a button press. */
void ui_view_press(const buttons_event_t* event);

/** Backlight percent chosen on the brightness page. */
uint8_t ui_view_brightness(void);
