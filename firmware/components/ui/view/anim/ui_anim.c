#include "ui_anim.h"
#include "ui_anim_internal.h"

static bool s_enabled = true;

void ui_anim_set_enabled(bool enabled)
{
    if (enabled == s_enabled) {
        return;
    }

    s_enabled = enabled;
    lv_obj_send_event(lv_screen_active(), enabled ? LV_EVENT_SCREEN_LOADED : LV_EVENT_SCREEN_UNLOAD_START, NULL);
}

bool ui_anim_enabled(void)
{
    return s_enabled;
}

void ui_anim_watch_screen(lv_obj_t* obj, lv_event_cb_t on_screen, void* user_data)
{
    lv_obj_t* screen = lv_obj_get_screen(obj);
    lv_obj_add_event_cb(screen, on_screen, LV_EVENT_SCREEN_LOADED, user_data);
    lv_obj_add_event_cb(screen, on_screen, LV_EVENT_SCREEN_UNLOAD_START, user_data);
}
