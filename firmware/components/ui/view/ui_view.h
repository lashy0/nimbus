#pragma once

#include "lvgl.h"

/**
 * @file
 * Screen content. Uses only LVGL, so it also builds for the simulator.
 * Call with exclusive LVGL access.
 */

/** Builds the start screen on the given LVGL screen. */
void ui_view_create(lv_obj_t* screen);
