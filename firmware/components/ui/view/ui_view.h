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

/** Keeps the snapshot, picks the page for it and updates that page. */
void ui_view_update(const sensing_snapshot_t* snapshot);

/**
 * Short presses switch the measurement pages. A long NEXT opens the brightness
 * page, where short presses change the brightness and a long NEXT closes it.
 */
void ui_view_press(const buttons_event_t* event);

/** Backlight percent chosen on the brightness page. */
uint8_t ui_view_brightness(void);
