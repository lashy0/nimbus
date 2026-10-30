#pragma once

#include <math.h>

#include "lvgl.h"

#include "sensing_types.h"

/**
 * @file
 * Pages of the UI. Each page builds its own LVGL screen once; pages with
 * changing content also have an update function that redraws only when the
 * shown value changes.
 */

/** Shown until the first measurement. */
lv_obj_t* ui_page_start_create(void);

lv_obj_t* ui_page_iaq_create(void);
void ui_page_iaq_update(const sensing_snapshot_t* snapshot);

lv_obj_t* ui_page_temp_create(void);
void ui_page_temp_update(const sensing_snapshot_t* snapshot);

lv_obj_t* ui_page_hum_create(void);
void ui_page_hum_update(const sensing_snapshot_t* snapshot);

/** Shown while the latest measurement has failed. */
lv_obj_t* ui_page_error_create(void);

/** Rounds a measurement to the whole number shown on a page. */
static inline int ui_page_round(float value, int min, int max)
{
    return (int)lroundf(fminf(fmaxf(value, (float)min), (float)max));
}
