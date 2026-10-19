#pragma once

#include "lvgl.h"
#include "ui_model.h"

typedef struct {
    const lv_font_t* digits_10;
    const lv_font_t* text_15;
    const lv_font_t* text_30;
    const lv_font_t* digits_50;
    const lv_font_t* digits_60;
} ui_fonts_t;

// Logs free, minimum and largest internal and DMA-capable heap at a named stage.
void ui_log_memory(const char* stage);

// Loads the runtime fonts from SPIFFS; a missing font falls back to the built-in one.
void ui_fonts_load(ui_fonts_t* fonts);

// Creates every widget on the screen once. Call with the LVGL lock held.
void ui_view_create(lv_obj_t* screen, const ui_fonts_t* fonts);

// Shows the model. Call with the LVGL lock held.
void ui_view_apply(const ui_model_t* model);

// Makes the face image blink. Call with the LVGL lock held.
void ui_blink_init(lv_obj_t* face);

// Shows open_src on the face and, when closed_src is not NULL, blinks with it at
// random intervals. Both are static strings; NULL open_src stops blinking.
void ui_blink_set(const char* open_src, const char* closed_src);
