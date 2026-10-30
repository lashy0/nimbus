#pragma once

#include "lvgl.h"

#include "sensing_types.h"

/**
 * @file
 * Screen content. Uses only LVGL, so it also builds for the simulator.
 * Call with exclusive LVGL access.
 */

/** Builds every page and shows the start page in place of the display's default screen. */
void ui_view_create(void);

/** Picks the page for the snapshot and updates it. */
void ui_view_update(const sensing_snapshot_t* snapshot);
