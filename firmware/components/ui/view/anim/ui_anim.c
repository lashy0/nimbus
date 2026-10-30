#include "ui_anim.h"
#include "ui_anim_internal.h"

void ui_anim_watch_screen(lv_obj_t* obj, lv_event_cb_t on_screen, void* user_data)
{
    lv_obj_t* screen = lv_obj_get_screen(obj);
    lv_obj_add_event_cb(screen, on_screen, LV_EVENT_SCREEN_LOADED, user_data);
    lv_obj_add_event_cb(screen, on_screen, LV_EVENT_SCREEN_UNLOAD_START, user_data);
}
