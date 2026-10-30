#pragma once

#include <stdint.h>

#include "lvgl.h"

/**
 * @file
 * Building blocks shared by the pages. Positions are in pixels from the
 * top-left corner of the screen.
 */

/** Creates a black page screen with white text; it is not shown yet. */
lv_obj_t* ui_screen_create(void);

lv_obj_t* ui_label_create(lv_obj_t* parent, const lv_font_t* font, const char* text, int32_t x, int32_t y);

/** Label centered across the full width of its parent. */
lv_obj_t* ui_centered_label_create(lv_obj_t* parent, const lv_font_t* font, const char* text, int32_t y);

lv_obj_t* ui_image_create(lv_obj_t* parent, const char* src, int32_t x, int32_t y);

/** Page indicator at the bottom; active is the index of the current page. */
void ui_dots_create(lv_obj_t* parent, uint32_t active);
