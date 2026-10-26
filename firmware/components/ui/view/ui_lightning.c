#include "ui_animation.h"

#include "esp_log.h"

#define FRAME_MS 40
#define PULSE_MS 1600

static const char* TAG = "ui_lightning";

static lv_obj_t* s_lightning;
static lv_timer_t* s_timer;
static uint32_t s_started_ms;
static bool s_visible;

static void lightning_step(lv_timer_t* timer)
{
    const uint32_t phase_ms = lv_tick_elaps(s_started_ms) % PULSE_MS;
    const int16_t angle = (int16_t)(phase_ms * 360 / PULSE_MS);
    const lv_opa_t opacity =
        (lv_opa_t)lv_map(lv_trigo_cos(angle), -LV_TRIGO_SIN_MAX, LV_TRIGO_SIN_MAX, LV_OPA_40, LV_OPA_COVER);
    lv_obj_set_style_image_opa(s_lightning, opacity, LV_PART_MAIN);
}

void ui_lightning_init(lv_obj_t* lightning)
{
    s_lightning = lightning;
    s_timer = lv_timer_create(lightning_step, FRAME_MS, NULL);
    if (s_timer == NULL) {
        ESP_LOGW(TAG, "lightning animation timer unavailable; using a static image");

        return;
    }

    lv_timer_pause(s_timer);
}

void ui_lightning_set(bool visible)
{
    if (s_timer == NULL || s_visible == visible) {
        return;
    }

    s_visible = visible;
    lv_obj_set_style_image_opa(s_lightning, LV_OPA_COVER, LV_PART_MAIN);
    if (!visible) {
        lv_timer_pause(s_timer);

        return;
    }

    // Model updates must not restart a pulse while the notice stays visible.
    s_started_ms = lv_tick_get();
    lv_timer_reset(s_timer);
    lv_timer_resume(s_timer);
}

void ui_lightning_deinit(void)
{
    if (s_timer != NULL) {
        lv_timer_delete(s_timer);
        s_timer = NULL;
    }

    s_lightning = NULL;
    s_started_ms = 0;
    s_visible = false;
}
