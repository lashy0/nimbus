#pragma once

#include <math.h>
#include <stdbool.h>

#include "lvgl.h"

#include "sensing_types.h"

/**
 * @file
 * Pages of the UI, one LVGL screen each.
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

lv_obj_t* ui_page_brightness_create(void);
void ui_page_brightness_update(uint8_t percent);

lv_obj_t* ui_page_power_off_create(void);
void ui_page_power_off_update(bool yes_selected);

/** Rounds a measurement to the whole number shown on a page. */
static inline int ui_page_round(float value, int min, int max)
{
    return (int)lroundf(fminf(fmaxf(value, (float)min), (float)max));
}
