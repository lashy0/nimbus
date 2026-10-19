#pragma once

// All screens share one LVGL screen: each page shows a subset of these widgets, so
// switching pages allocates nothing and redraws only what changed. Call everything
// here with the LVGL lock held.
#include <stdbool.h>
#include <stdint.h>

#include "ui_internal.h"

#define UI_IMG(path) "S:/spiffs/img/" path ".bin"

typedef enum {
    UI_W_BATTERY_TEXT,
    UI_W_BATTERY,
    UI_W_TITLE,
    UI_W_PLATE,
    UI_W_VALUE,
    UI_W_FACE,
    UI_W_CAPTION,
    UI_W_SUN,
    UI_W_BAR,
    UI_W_ANSWERS,
    UI_W_DOTS,
    UI_W_COUNT,
} ui_widget_t;

#define UI_SHOW(widget) (1u << (widget))

typedef enum {
    UI_FACE_BASE,
    UI_FACE_ULTRA_HAPPY,
    UI_FACE_HAPPY,
    UI_FACE_ORDINARY,
    UI_FACE_NIMBUS,
    UI_FACE_SAD,
    UI_FACE_DIZZY,
    UI_FACE_DEAD,
    UI_FACE_DIVER,
    UI_FACE_HUH,
    UI_FACE_TEMP_MINUS,
    UI_FACE_TEMP_NORMAL,
    UI_FACE_TEMP_PLUS,
} ui_face_t;

// Creates every widget hidden.
void ui_widgets_create(lv_obj_t* screen, const ui_fonts_t* fonts);

// Each ui_show_* fills its widget and returns its UI_SHOW() bit; only changed
// properties are touched.
uint32_t ui_show_battery(const char* text, const char* icon_src, lv_coord_t icon_y);
uint32_t ui_show_title(const char* text, lv_coord_t x, lv_coord_t y);
uint32_t ui_show_plate(const char* src, lv_coord_t x, lv_coord_t y);
uint32_t ui_show_value(const char* text, const lv_font_t* font);
uint32_t ui_show_caption(const char* text, lv_coord_t y);
uint32_t ui_show_bar(uint8_t percent);
uint32_t ui_show_answers(bool yes_selected);
uint32_t ui_show_dots(size_t active);

// open_src and closed_src as for ui_blink_set().
uint32_t ui_show_face_image(const char* open_src, const char* closed_src, lv_coord_t x, lv_coord_t y);
uint32_t ui_show_face(ui_face_t face);

// Shows the widgets whose bits are set and hides the rest.
void ui_widgets_set_visible(uint32_t visible);
