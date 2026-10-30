#include <limits.h>

#include "ui_anim.h"
#include "ui_assets.h"
#include "ui_font.h"
#include "ui_page.h"
#include "ui_widgets.h"

#define HUMIDITY_MAX 100
#define DRY_BELOW 30
#define DAMP_FROM 70

static lv_obj_t* s_plate;
static lv_obj_t* s_value;
static ui_blink_t* s_blink;
static int s_shown = INT_MIN; // Rounded humidity on screen.

lv_obj_t* ui_page_hum_create(void)
{
    lv_obj_t* screen = ui_screen_create();
    ui_label_create(screen, ui_font_semibold_30, "Hum", 4, 42);
    s_value = ui_centered_label_create(screen, ui_font_semibold_60, "", 72);
    ui_dots_create(screen, 2);

    s_plate = lv_image_create(screen);
    lv_obj_set_pos(s_plate, 73, 34);
    lv_obj_t* face = lv_image_create(screen);
    lv_obj_set_pos(face, 32, 137);
    s_blink = ui_blink_create(face);

    return screen;
}

void ui_page_hum_update(const sensing_snapshot_t* snapshot)
{
    const int percent = ui_page_round(snapshot->humidity_percent, 0, HUMIDITY_MAX);
    if (percent == s_shown) {
        return;
    }

    s_shown = percent;
    lv_label_set_text_fmt(s_value, "%d%%", percent);

    // "100%" does not fit the screen width at 60 px.
    const lv_font_t* font = percent >= 100 ? ui_font_semibold_50 : ui_font_semibold_60;
    lv_obj_set_style_text_font(s_value, font, 0);

    if (percent < DRY_BELOW) {
        lv_image_set_src(s_plate, UI_IMAGE("status/dry"));
        ui_blink_set(s_blink, UI_IMAGE("face/ordinary"), UI_IMAGE("face/ordinary_closed_eyes"));
    } else if (percent < DAMP_FROM) {
        lv_image_set_src(s_plate, UI_IMAGE("status/good"));
        ui_blink_set(s_blink, UI_IMAGE("face/happy"), UI_IMAGE("face/happy_closed_eyes"));
    } else {
        lv_image_set_src(s_plate, UI_IMAGE("status/damp"));
        ui_blink_set(s_blink, UI_IMAGE("face/diver"), UI_IMAGE("face/diver_closed_eyes"));
    }
}
