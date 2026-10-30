#include "ui_anim.h"
#include "ui_anim_internal.h"

#define CLOSED_MS 140
#define REOPEN_MS 160 // Open eyes between the two halves of a double blink.
#define INTERVAL_MIN_MS 2500
#define INTERVAL_MAX_MS 6500
#define DOUBLE_PERCENT 20

struct ui_blink {
    lv_obj_t* face;
    lv_timer_t* timer;
    const char* open;
    const char* closed;
    bool eyes_closed;
    bool second_due; // The current blink is double.
};

static void wait_for_next(ui_blink_t* blink)
{
    blink->second_due = lv_rand(0, 99) < DOUBLE_PERCENT;
    lv_timer_set_period(blink->timer, lv_rand(INTERVAL_MIN_MS, INTERVAL_MAX_MS));
    lv_timer_reset(blink->timer);
}

static void on_step(lv_timer_t* timer)
{
    ui_blink_t* blink = lv_timer_get_user_data(timer);
    blink->eyes_closed = !blink->eyes_closed;
    lv_image_set_src(blink->face, blink->eyes_closed ? blink->closed : blink->open);

    if (blink->eyes_closed) {
        lv_timer_set_period(timer, CLOSED_MS);
    } else if (blink->second_due) {
        blink->second_due = false;
        lv_timer_set_period(timer, REOPEN_MS);
    } else {
        wait_for_next(blink);
    }
}

static void start(ui_blink_t* blink)
{
    if (blink->closed != NULL) {
        wait_for_next(blink);
        lv_timer_resume(blink->timer);
    }
}

static void stop(ui_blink_t* blink)
{
    lv_timer_pause(blink->timer);
    if (blink->eyes_closed) {
        blink->eyes_closed = false;
        lv_image_set_src(blink->face, blink->open);
    }
}

static void on_screen(lv_event_t* event)
{
    ui_blink_t* blink = lv_event_get_user_data(event);
    if (lv_event_get_code(event) == LV_EVENT_SCREEN_LOADED) {
        start(blink);
    } else {
        stop(blink);
    }
}

ui_blink_t* ui_blink_create(lv_obj_t* face)
{
    ui_blink_t* blink = lv_zalloc(sizeof(*blink));
    LV_ASSERT_MALLOC(blink);

    blink->face = face;
    blink->timer = lv_timer_create(on_step, INTERVAL_MAX_MS, blink);
    lv_timer_pause(blink->timer);
    ui_anim_watch_screen(face, on_screen, blink);

    return blink;
}

void ui_blink_set(ui_blink_t* blink, const char* open, const char* closed)
{
    lv_timer_pause(blink->timer);
    blink->open = open;
    blink->closed = closed;
    blink->eyes_closed = false;
    lv_image_set_src(blink->face, open);

    if (lv_screen_active() == lv_obj_get_screen(blink->face)) {
        start(blink);
    }
}
