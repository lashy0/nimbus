#pragma once

#include "lvgl.h"

/**
 * @file
 * Animations of page images. Each runs only while the screen holding its
 * image is shown.
 */

typedef struct ui_blink ui_blink_t;

/** Blinking for a face image; the face stays still until ui_blink_set(). */
ui_blink_t* ui_blink_create(lv_obj_t* face);

/**
 * Shows a face and blinks it now and then.
 *
 * @param open Image source with open eyes.
 * @param closed Image source with closed eyes, or NULL to keep the face still.
 */
void ui_blink_set(ui_blink_t* blink, const char* open, const char* closed);

/** Makes the image fade to 40 % opacity and back, 1.6 s per cycle. */
void ui_pulse_create(lv_obj_t* image);
