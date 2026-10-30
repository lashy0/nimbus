#include "ui_view.h"

#include "ui_battery.h"
#include "ui_page.h"

#define BRIGHTNESS_MIN 5
#define BRIGHTNESS_MAX 100
#define BRIGHTNESS_STEP 5
#define BRIGHTNESS_DEFAULT 40

#define QUESTION_TIMEOUT_MS 15000 // Without presses.

typedef struct {
    lv_obj_t* (*create)(void);
    void (*update)(const sensing_snapshot_t* snapshot);
} page_t;

typedef enum {
    MODE_MEASUREMENT,
    MODE_BRIGHTNESS,
    MODE_POWER_OFF_QUESTION,
    MODE_POWER_OFF_CONFIRMED,
} view_mode_t;

// Measurement pages in the order NEXT walks them.
static const page_t PAGES[] = {
    {ui_page_iaq_create, ui_page_iaq_update},
    {ui_page_temp_create, ui_page_temp_update},
    {ui_page_hum_create, ui_page_hum_update},
};

#define PAGE_COUNT (sizeof(PAGES) / sizeof(PAGES[0]))

static lv_obj_t* s_start;
static lv_obj_t* s_error;
static lv_obj_t* s_brightness_page;
static lv_obj_t* s_power_off_page;
static lv_obj_t* s_pages[PAGE_COUNT];
static size_t s_selected;
static sensing_snapshot_t s_snapshot; // NO_DATA until the first update.
static view_mode_t s_mode;
static uint8_t s_brightness;
static bool s_yes_selected;
static lv_timer_t* s_question_timer;

static bool brightness_valid(uint8_t percent)
{
    return percent >= BRIGHTNESS_MIN && percent <= BRIGHTNESS_MAX && percent % BRIGHTNESS_STEP == 0;
}

static void show(lv_obj_t* page)
{
    if (lv_screen_active() != page) {
        lv_screen_load(page);
        ui_battery_set_visible(page != s_start);
    }
}

static void refresh(void)
{
    switch (s_mode) {
        case MODE_BRIGHTNESS:
            ui_page_brightness_update(s_brightness);
            show(s_brightness_page);

            return;
        case MODE_POWER_OFF_QUESTION:
        case MODE_POWER_OFF_CONFIRMED:
            ui_page_power_off_update(s_yes_selected);
            show(s_power_off_page);

            return;
        case MODE_MEASUREMENT:
            break;
    }

    switch (s_snapshot.status) {
        case SENSING_STATUS_NO_DATA:
            show(s_start);
            break;
        case SENSING_STATUS_OK:
            PAGES[s_selected].update(&s_snapshot);
            show(s_pages[s_selected]);
            break;
        case SENSING_STATUS_ERROR:
            show(s_error);
            break;
    }
}

static void close_question(void)
{
    lv_timer_pause(s_question_timer);
    s_mode = MODE_MEASUREMENT;
}

static void on_question_timeout(lv_timer_t* timer)
{
    close_question();
    refresh();
}

void ui_view_create(uint8_t brightness)
{
    s_brightness = brightness_valid(brightness) ? brightness : BRIGHTNESS_DEFAULT;

    s_start = ui_page_start_create();
    s_error = ui_page_error_create();
    s_brightness_page = ui_page_brightness_create();
    s_power_off_page = ui_page_power_off_create();
    for (size_t i = 0; i < PAGE_COUNT; i++) {
        s_pages[i] = PAGES[i].create();
    }

    s_question_timer = lv_timer_create(on_question_timeout, QUESTION_TIMEOUT_MS, NULL);
    lv_timer_pause(s_question_timer);

    ui_battery_create();
    ui_battery_set_visible(false);

    lv_obj_t* initial = lv_screen_active();
    lv_screen_load(s_start);
    lv_obj_delete(initial);
}

void ui_view_update(const sensing_snapshot_t* snapshot)
{
    s_snapshot = *snapshot;
    refresh();
}

static void press_brightness(const buttons_event_t* event)
{
    if (event->press == BUTTONS_PRESS_LONG) {
        if (event->id == BUTTONS_NEXT) {
            s_mode = MODE_MEASUREMENT;
        }

        return;
    }

    const int step = event->id == BUTTONS_NEXT ? BRIGHTNESS_STEP : -BRIGHTNESS_STEP;
    const int percent = s_brightness + step;
    if (percent >= BRIGHTNESS_MIN && percent <= BRIGHTNESS_MAX) {
        s_brightness = (uint8_t)percent;
    }
}

// PREV stands under the yes answer, NEXT under no.
static void press_question(const buttons_event_t* event)
{
    if (event->press == BUTTONS_PRESS_LONG) {
        return;
    }

    lv_timer_reset(s_question_timer);

    const bool yes = event->id == BUTTONS_PREV;
    if (yes != s_yes_selected) {
        s_yes_selected = yes;
    } else if (yes) {
        lv_timer_pause(s_question_timer);
        s_mode = MODE_POWER_OFF_CONFIRMED;
    } else {
        close_question();
    }
}

static void press_measurement(const buttons_event_t* event)
{
    if (event->press == BUTTONS_PRESS_LONG) {
        if (event->id == BUTTONS_NEXT) {
            s_mode = MODE_BRIGHTNESS;
        } else {
            s_mode = MODE_POWER_OFF_QUESTION;
            s_yes_selected = true;
            lv_timer_reset(s_question_timer);
            lv_timer_resume(s_question_timer);
        }

        return;
    }

    if (s_snapshot.status == SENSING_STATUS_OK) {
        const size_t step = event->id == BUTTONS_NEXT ? 1 : PAGE_COUNT - 1;
        s_selected = (s_selected + step) % PAGE_COUNT;
    }
}

void ui_view_press(const buttons_event_t* event)
{
    switch (s_mode) {
        case MODE_MEASUREMENT:
            press_measurement(event);
            break;
        case MODE_BRIGHTNESS:
            press_brightness(event);
            break;
        case MODE_POWER_OFF_QUESTION:
            press_question(event);
            break;
        case MODE_POWER_OFF_CONFIRMED:
            return;
    }

    refresh();
}

void ui_view_update_power(const power_status_t* status)
{
    ui_battery_update(status);
}

uint8_t ui_view_brightness(void)
{
    return s_brightness;
}

bool ui_view_power_off_confirmed(void)
{
    return s_mode == MODE_POWER_OFF_CONFIRMED;
}
