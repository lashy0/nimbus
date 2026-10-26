#pragma once

#include "lvgl.h"

// Call under the renderer lock. A missing timer leaves a static image.
void ui_blink_init(lv_obj_t* face);
void ui_charge_init(lv_obj_t* battery);
void ui_lightning_init(lv_obj_t* lightning);

// Image paths and frame arrays are borrowed and must outlive the animation.
// NULL stops the animation; a NULL closed image disables blinking.
void ui_blink_set(const char* open_src, const char* closed_src);
void ui_charge_set(const char* const* frames, size_t count);

// Pulses only while visible; repeated visibility updates keep the current phase.
void ui_lightning_set(bool visible);

// Delete timers before deleting their target widgets.
void ui_blink_deinit(void);
void ui_charge_deinit(void);
void ui_lightning_deinit(void);
