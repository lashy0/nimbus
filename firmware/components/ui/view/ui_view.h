#pragma once

#include "lvgl.h"

#include "buttons_types.h"
#include "sensing_types.h"

/**
 * @file
 * Screen content. Uses only LVGL, so it also builds for the simulator.
 * Call with exclusive LVGL access.
 */

/** Builds every page and shows the start page in place of the display's default screen. */
void ui_view_create(void);

/** Keeps the snapshot, picks the page for it and updates that page. */
void ui_view_update(const sensing_snapshot_t* snapshot);

/** Short presses switch the measurement pages; elsewhere presses are ignored. */
void ui_view_press(const buttons_event_t* event);
