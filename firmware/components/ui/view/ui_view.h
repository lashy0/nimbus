#pragma once

#include "esp_err.h"
#include "ui_fonts.h"
#include "ui_model.h"

// All view operations require the renderer lock. Widgets are owned by screen.
esp_err_t ui_view_create(lv_obj_t* screen, const ui_fonts_t* fonts);
void ui_view_apply(const ui_model_t* model);

// Stops animations before their widgets are deleted.
void ui_view_deinit(void);
