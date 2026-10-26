#pragma once

#include "lvgl.h"

typedef struct {
    const lv_font_t* digits_10;
    const lv_font_t* text_15;
    const lv_font_t* text_30;
    const lv_font_t* digits_50;
    const lv_font_t* digits_60;
} ui_fonts_t;

// Call under the renderer lock. Missing fonts use the built-in font.
void ui_fonts_load(ui_fonts_t* fonts);

// Call after deleting widgets that use these fonts; the built-in font is borrowed.
void ui_fonts_unload(ui_fonts_t* fonts);
