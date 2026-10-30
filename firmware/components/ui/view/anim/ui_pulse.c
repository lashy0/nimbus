#include "ui_anim.h"
#include "ui_anim_internal.h"

#define FADE_MS 800 // Each way.
#define OPA_MIN LV_OPA_40

static void set_opa(void* image, int32_t opa)
{
    lv_obj_set_style_image_opa(image, (lv_opa_t)opa, 0);
}

static void start(lv_obj_t* image)
{
    lv_anim_delete(image, set_opa);

    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, image);
    lv_anim_set_exec_cb(&anim, set_opa);
    lv_anim_set_values(&anim, LV_OPA_COVER, OPA_MIN);
    lv_anim_set_duration(&anim, FADE_MS);
    lv_anim_set_reverse_duration(&anim, FADE_MS);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
    lv_anim_start(&anim);
}

static void stop(lv_obj_t* image)
{
    lv_anim_delete(image, set_opa);
    set_opa(image, LV_OPA_COVER);
}

static void on_screen(lv_event_t* event)
{
    lv_obj_t* image = lv_event_get_user_data(event);
    if (lv_event_get_code(event) == LV_EVENT_SCREEN_LOADED) {
        start(image);
    } else {
        stop(image);
    }
}

void ui_pulse_create(lv_obj_t* image)
{
    ui_anim_watch_screen(image, on_screen, image);
}
