#include "ui_animation.h"

#include "esp_log.h"

#define STEP_MS 500

static const char* TAG = "ui_charge";

static lv_obj_t* s_battery;
static lv_timer_t* s_timer;
static const char* const* s_frames;
static size_t s_count;
static size_t s_index;

static void charge_step(lv_timer_t* timer)
{
    s_index = (s_index + 1) % s_count;
    lv_image_set_src(s_battery, s_frames[s_index]);
}

void ui_charge_init(lv_obj_t* battery)
{
    s_battery = battery;
    s_timer = lv_timer_create(charge_step, STEP_MS, NULL);
    if (s_timer == NULL) {
        ESP_LOGW(TAG, "battery animation timer unavailable; using the first frame");

        return;
    }

    lv_timer_pause(s_timer);
}

void ui_charge_deinit(void)
{
    if (s_timer != NULL) {
        lv_timer_delete(s_timer);
        s_timer = NULL;
    }

    s_battery = NULL;
    s_frames = NULL;
    s_count = 0;
    s_index = 0;
}

void ui_charge_set(const char* const* frames, size_t count)
{
    if (frames == s_frames && count == s_count) {
        return;
    }

    s_frames = frames;
    s_count = count;
    if (frames == NULL || count == 0) {
        if (s_timer != NULL) {
            lv_timer_pause(s_timer);
        }

        return;
    }

    s_index = 0;
    lv_image_set_src(s_battery, frames[0]);
    if (s_timer == NULL) {
        return;
    }

    lv_timer_reset(s_timer);
    lv_timer_resume(s_timer);
}
