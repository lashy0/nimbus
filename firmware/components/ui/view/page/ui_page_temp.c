#include <limits.h>

#include "ui_anim.h"
#include "ui_assets.h"
#include "ui_font.h"
#include "ui_page.h"
#include "ui_widgets.h"

// BME680 operating range.
#define TEMPERATURE_MIN_C -40
#define TEMPERATURE_MAX_C 85
#define NORMAL_MAX_C 25

static lv_obj_t* s_plate;
static lv_obj_t* s_value;
static ui_blink_t* s_blink;
static int s_shown = INT_MIN; // Rounded temperature on screen.

lv_obj_t* ui_page_temp_create(void)
{
    lv_obj_t* screen = ui_screen_create();
    ui_label_create(screen, ui_font_semibold_30, "Temp", 17, 42);
    s_value = ui_centered_label_create(screen, ui_font_semibold_60, "", 72);
    ui_dots_create(screen, 1);

    s_plate = lv_image_create(screen);
    lv_obj_t* face = lv_image_create(screen);
    lv_obj_set_pos(face, 31, 127);
    s_blink = ui_blink_create(face);

    return screen;
}

static void show_level(const char* plate, int32_t plate_x, const char* face, const char* face_closed)
{
    lv_image_set_src(s_plate, plate);
    lv_obj_set_pos(s_plate, plate_x, 41);
    ui_blink_set(s_blink, face, face_closed);
}

void ui_page_temp_update(const sensing_snapshot_t* snapshot)
{
    const int celsius = ui_page_round(snapshot->temperature_c, TEMPERATURE_MIN_C, TEMPERATURE_MAX_C);
    if (celsius == s_shown) {
        return;
    }

    s_shown = celsius;
    lv_label_set_text_fmt(s_value, "%d°", celsius);

    if (celsius < 0) {
        show_level(UI_IMAGE("status/minus"), 97, UI_IMAGE("face/temp_minus"), UI_IMAGE("face/temp_minus_closed_eyes"));
    } else if (celsius <= NORMAL_MAX_C) {
        show_level(
            UI_IMAGE("status/nothing"), 99, UI_IMAGE("face/temp_normal"), UI_IMAGE("face/temp_normal_closed_eyes"));
    } else {
        show_level(UI_IMAGE("status/plus"), 97, UI_IMAGE("face/temp_plus"), NULL);
    }
}
