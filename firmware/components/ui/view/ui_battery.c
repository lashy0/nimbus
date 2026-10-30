#include "ui_battery.h"

#include "ui_assets.h"
#include "ui_font.h"
#include "ui_widgets.h"

#define CHARGE_STEP_MS 500

// Values of s_shown besides a percent.
#define SHOWN_UNKNOWN -2
#define SHOWN_EXTERNAL -1

typedef struct {
    int min_percent;
    const char* icon;
} level_t;

static const level_t LEVELS[] = {
    {0, UI_IMAGE("batt/1_not_charging")},
    {30, UI_IMAGE("batt/2_not_charging")},
    {60, UI_IMAGE("batt/3_not_charging")},
    {90, UI_IMAGE("batt/full_not_charging")},
};

#define LEVEL_COUNT (sizeof(LEVELS) / sizeof(LEVELS[0]))

// Cycled on USB.
static const char* const CHARGE_FRAMES[] = {
    UI_IMAGE("batt/1_charging"),
    UI_IMAGE("batt/2_charging"),
    UI_IMAGE("batt/3_charging"),
    UI_IMAGE("batt/full_charging"),
};

#define CHARGE_FRAME_COUNT (sizeof(CHARGE_FRAMES) / sizeof(CHARGE_FRAMES[0]))

static lv_obj_t* s_box;
static lv_obj_t* s_text;
static lv_obj_t* s_icon;
static lv_timer_t* s_charge_timer;
static size_t s_frame;
static int s_shown = SHOWN_UNKNOWN;

static void on_charge_step(lv_timer_t* timer)
{
    s_frame = (s_frame + 1) % CHARGE_FRAME_COUNT;
    lv_image_set_src(s_icon, CHARGE_FRAMES[s_frame]);
}

void ui_battery_create(void)
{
    s_box = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(s_box);
    lv_obj_set_size(s_box, lv_pct(100), 30);
    lv_obj_set_style_text_color(s_box, lv_color_white(), 0);

    s_text = ui_label_create(s_box, ui_font_bold_10, "-- %", 41, 12);
    lv_obj_set_width(s_text, 30);
    lv_obj_set_style_text_align(s_text, LV_TEXT_ALIGN_RIGHT, 0);

    s_icon = lv_image_create(s_box);
    lv_obj_add_flag(s_icon, LV_OBJ_FLAG_HIDDEN);

    s_charge_timer = lv_timer_create(on_charge_step, CHARGE_STEP_MS, NULL);
    lv_timer_pause(s_charge_timer);
}

static const char* level_icon(int percent)
{
    for (size_t i = LEVEL_COUNT - 1; i > 0; i--) {
        if (percent >= LEVELS[i].min_percent) {
            return LEVELS[i].icon;
        }
    }

    return LEVELS[0].icon;
}

static void show_external(void)
{
    lv_obj_add_flag(s_text, LV_OBJ_FLAG_HIDDEN);
    s_frame = 0;
    lv_image_set_src(s_icon, CHARGE_FRAMES[0]);
    lv_obj_set_pos(s_icon, 57, 8);
    lv_obj_remove_flag(s_icon, LV_OBJ_FLAG_HIDDEN);

    lv_timer_reset(s_charge_timer);
    lv_timer_resume(s_charge_timer);
}

static void show_percent(int percent)
{
    lv_label_set_text_fmt(s_text, "%d %%", percent);
    lv_obj_remove_flag(s_text, LV_OBJ_FLAG_HIDDEN);
    lv_image_set_src(s_icon, level_icon(percent));
    lv_obj_set_pos(s_icon, 74, 11);
    lv_obj_remove_flag(s_icon, LV_OBJ_FLAG_HIDDEN);
}

static void show_unknown(void)
{
    lv_label_set_text(s_text, "-- %");
    lv_obj_remove_flag(s_text, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_icon, LV_OBJ_FLAG_HIDDEN);
}

void ui_battery_update(const power_status_t* status)
{
    int shown = SHOWN_UNKNOWN;
    if (status->source == POWER_SOURCE_EXTERNAL) {
        shown = SHOWN_EXTERNAL;
    } else if (status->source == POWER_SOURCE_BATTERY && status->percent_valid) {
        shown = status->percent;
    }

    if (shown == s_shown) {
        return;
    }

    s_shown = shown;
    if (shown == SHOWN_EXTERNAL) {
        show_external();

        return;
    }

    lv_timer_pause(s_charge_timer);
    if (shown == SHOWN_UNKNOWN) {
        show_unknown();
    } else {
        show_percent(shown);
    }
}

void ui_battery_set_visible(bool visible)
{
    if (visible) {
        lv_obj_remove_flag(s_box, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_box, LV_OBJ_FLAG_HIDDEN);
    }
}
