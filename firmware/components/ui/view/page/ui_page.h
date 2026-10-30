#pragma once

#include "lvgl.h"

#include "sensing_types.h"

/**
 * @file
 * Pages of the UI. Each page builds its own LVGL screen once; pages with
 * changing content also have an update function.
 */

/** Shown until the first measurement. */
lv_obj_t* ui_page_start_create(void);

lv_obj_t* ui_page_iaq_create(void);

/** Redraws only when the shown value changes. */
void ui_page_iaq_update(const sensing_snapshot_t* snapshot);

/** Shown while the latest measurement has failed. */
lv_obj_t* ui_page_error_create(void);
