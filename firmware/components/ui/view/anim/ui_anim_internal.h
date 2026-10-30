#pragma once

#include <stdbool.h>

#include "lvgl.h"

/** Set by ui_anim_set_enabled(). */
bool ui_anim_enabled(void);

/**
 * Calls on_screen with LV_EVENT_SCREEN_LOADED or LV_EVENT_SCREEN_UNLOAD_START
 * as the screen holding obj is shown or left, and as animations are enabled
 * or disabled while it is shown.
 */
void ui_anim_watch_screen(lv_obj_t* obj, lv_event_cb_t on_screen, void* user_data);
