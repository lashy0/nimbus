#pragma once

#include "lvgl.h"

/**
 * @file
 * Fonts from the assets partition, kept in RAM once loaded. Each holds only
 * the glyphs the pages use.
 */

extern const lv_font_t* ui_font_bold_10;     /**< Digits, space, % and -. */
extern const lv_font_t* ui_font_semibold_15; /**< ASCII and Cyrillic. */
extern const lv_font_t* ui_font_semibold_30; /**< Latin letters. */
extern const lv_font_t* ui_font_semibold_50; /**< Digits and %. */
extern const lv_font_t* ui_font_semibold_60; /**< Digits, %, - and °. */

/** Loads every font; one that cannot be loaded is replaced by LVGL's default font. */
void ui_font_load(void);
