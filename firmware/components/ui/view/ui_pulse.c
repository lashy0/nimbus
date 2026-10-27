#include "ui_animation.h"

#define FRAME_MS 40
#define PULSE_MS 1600

static lv_obj_t* s_image;
static lv_timer_t* s_timer;
static uint32_t s_started_ms;

static void pulse_step(lv_timer_t* timer)
{
    const uint32_t phase_ms = lv_tick_elaps(s_started_ms) % PULSE_MS;
    const int16_t angle = (int16_t)(phase_ms * 360 / PULSE_MS);
    const lv_opa_t opacity =
        (lv_opa_t)lv_map(lv_trigo_cos(angle), -LV_TRIGO_SIN_MAX, LV_TRIGO_SIN_MAX, LV_OPA_40, LV_OPA_COVER);
    lv_obj_set_style_image_opa(s_image, opacity, LV_PART_MAIN);
}

void ui_pulse_init(void)
{
    s_timer = lv_timer_create(pulse_step, FRAME_MS, NULL);
    if (s_timer == NULL) {
        LV_LOG_WARN("pulse animation timer unavailable; using static images");

        return;
    }

    lv_timer_pause(s_timer);
}

void ui_pulse_set(lv_obj_t* image)
{
    if (s_image == image) {
        return;
    }

    if (s_image != NULL) {
        lv_obj_set_style_image_opa(s_image, LV_OPA_COVER, LV_PART_MAIN);
    }

    s_image = image;
    if (s_timer == NULL) {
        return;
    }

    if (image == NULL) {
        lv_timer_pause(s_timer);

        return;
    }

    // Repeated models keep the current phase; changing the image starts at full opacity.
    s_started_ms = lv_tick_get();
    lv_obj_set_style_image_opa(s_image, LV_OPA_COVER, LV_PART_MAIN);
    lv_timer_reset(s_timer);
    lv_timer_resume(s_timer);
}

void ui_pulse_deinit(void)
{
    if (s_timer != NULL) {
        lv_timer_delete(s_timer);
        s_timer = NULL;
    }

    s_image = NULL;
    s_started_ms = 0;
}
