#pragma once

#include "lvgl.h"

/**
 * @file
 * Fonts compiled into the firmware from artwork/fonts with lv_font_conv.
 * Each file keeps its command in the Opts line; run it from the repository root.
 */

LV_FONT_DECLARE(ui_font_semibold_15); /**< Latin and Cyrillic text. */
LV_FONT_DECLARE(ui_font_semibold_30); /**< Latin letters for titles. */
LV_FONT_DECLARE(ui_font_semibold_60); /**< Digits, minus, percent and degree. */
