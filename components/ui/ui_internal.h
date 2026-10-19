#pragma once

#include "lvgl.h"

// Logs free, minimum and largest internal and DMA-capable heap at a named stage.
void ui_log_memory(const char* stage);

// Fills the screen with the bring-up view: edge markers, every image in turn and
// all runtime fonts. Call with the LVGL lock held.
void ui_bringup_create(lv_obj_t* screen);
