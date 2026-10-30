#pragma once

#include "lvgl.h"

/**
 * Calls on_screen with LV_EVENT_SCREEN_LOADED or LV_EVENT_SCREEN_UNLOAD_START
 * as the screen holding obj is shown or left.
 */
void ui_anim_watch_screen(lv_obj_t* obj, lv_event_cb_t on_screen, void* user_data);
