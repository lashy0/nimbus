#include "ui_animation.h"

#define CLOSED_MS 140
#define REOPEN_GAP_MS 160 // Open eyes between the two halves of a double blink.
#define INTERVAL_MIN_MS 2500
#define INTERVAL_MAX_MS 6500
#define DOUBLE_BLINK_PERCENT 20

static lv_obj_t* s_face;
static lv_timer_t* s_timer;
static const char* s_open_src;
static const char* s_closed_src;
static bool s_closed;
static bool s_second_blink_due;

static void wait_for_next_blink(void)
{
    s_second_blink_due = lv_rand(0, 99) < DOUBLE_BLINK_PERCENT;
    lv_timer_set_period(s_timer, lv_rand(INTERVAL_MIN_MS, INTERVAL_MAX_MS));
    lv_timer_reset(s_timer);
}

static void blink_step(lv_timer_t* timer)
{
    if (!s_closed) {
        lv_image_set_src(s_face, s_closed_src);
        s_closed = true;
        lv_timer_set_period(timer, CLOSED_MS);

        return;
    }

    lv_image_set_src(s_face, s_open_src);
    s_closed = false;

    if (s_second_blink_due) {
        s_second_blink_due = false;
        lv_timer_set_period(timer, REOPEN_GAP_MS);

        return;
    }

    wait_for_next_blink();
}

void ui_blink_init(lv_obj_t* face)
{
    s_face = face;
    s_timer = lv_timer_create(blink_step, INTERVAL_MAX_MS, NULL);
    if (s_timer == NULL) {
        LV_LOG_WARN("blink timer unavailable; using a static face");

        return;
    }

    lv_timer_pause(s_timer);
}

void ui_blink_deinit(void)
{
    if (s_timer != NULL) {
        lv_timer_delete(s_timer);
        s_timer = NULL;
    }

    s_face = NULL;
    s_open_src = NULL;
    s_closed_src = NULL;
    s_closed = false;
    s_second_blink_due = false;
}

void ui_blink_set(const char* open_src, const char* closed_src)
{
    if (open_src == s_open_src && closed_src == s_closed_src) {
        return;
    }

    s_open_src = open_src;
    s_closed_src = closed_src;
    s_closed = false;

    if (open_src != NULL) {
        lv_image_set_src(s_face, open_src);
    }

    if (s_timer == NULL) {
        return;
    }

    if (open_src == NULL || closed_src == NULL) {
        lv_timer_pause(s_timer);

        return;
    }

    wait_for_next_blink();
    lv_timer_resume(s_timer);
}
