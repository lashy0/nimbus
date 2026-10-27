#pragma once

#include "ui_fonts.h"
#include "ui_model.h"

/**
 * Creates widgets owned by screen, borrowing fonts until the widgets are deleted.
 * All view calls require exclusive LVGL access (the renderer lock on ESP32).
 * @retval LV_RESULT_OK All widgets were created.
 * @retval LV_RESULT_INVALID Allocation failed; call ui_view_deinit() and delete screen
 * before unloading fonts, including after a partially completed creation.
 */
lv_result_t ui_view_create(lv_obj_t* screen, const ui_fonts_t* fonts);

/** Applies a complete model to the existing widgets under exclusive LVGL access. */
void ui_view_apply(const ui_model_t* model);

/** Stops animations under exclusive LVGL access, before deleting their widgets. */
void ui_view_deinit(void);
