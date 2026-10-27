#pragma once

#include "lvgl.h"

// Call under the renderer lock. A missing timer leaves a static image.
void ui_blink_init(lv_obj_t* face);
void ui_charge_init(lv_obj_t* battery);

/** Creates a paused shared pulse timer under the LVGL lock; allocation failure leaves static images. */
void ui_pulse_init(void);

// Image paths and frame arrays are borrowed and must outlive the animation.
// NULL stops the animation; a NULL closed image disables blinking.
void ui_blink_set(const char* open_src, const char* closed_src);
void ui_charge_set(const char* const* frames, size_t count);

/**
 * Pulses a borrowed image under the LVGL lock after ui_pulse_init(); NULL stops the timer.
 * Reusing the same target preserves phase. Changing/stopping the target restores its full opacity.
 * The target must remain alive until it is replaced, stopped or ui_pulse_deinit() is called.
 */
void ui_pulse_set(lv_obj_t* image);

// Delete timers before deleting their target widgets.
void ui_blink_deinit(void);
void ui_charge_deinit(void);

/** Deletes the pulse timer under the LVGL lock before its target widget is deleted. */
void ui_pulse_deinit(void);
