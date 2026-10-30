#include "ui_assets.h"
#include "ui_font.h"
#include "ui_page.h"
#include "ui_widgets.h"

#define IAQ_MAX 500

// Values of s_shown besides a rounded IAQ.
#define SHOWN_NOTHING -2
#define SHOWN_CALIBRATING -1

typedef struct {
    int max_iaq;
    const char* face;
    const char* plate;
} level_t;

static const level_t LEVELS[] = {
    {50, UI_IMAGE("face/ultra_happy"), UI_IMAGE("status/good")},
    {100, UI_IMAGE("face/happy"), UI_IMAGE("status/good")},
    {200, UI_IMAGE("face/ordinary"), UI_IMAGE("status/mid")},
    {300, UI_IMAGE("face/sad"), UI_IMAGE("status/bad")},
    {400, UI_IMAGE("face/dizzy"), UI_IMAGE("status/warn")},
    {IAQ_MAX, UI_IMAGE("face/dead"), UI_IMAGE("status/crit")},
};

#define LEVEL_COUNT (sizeof(LEVELS) / sizeof(LEVELS[0]))

static lv_obj_t* s_title;
static lv_obj_t* s_plate;
static lv_obj_t* s_value;
static lv_obj_t* s_face;
static lv_obj_t* s_caption;
static int s_shown = SHOWN_NOTHING;

lv_obj_t* ui_page_iaq_create(void)
{
    lv_obj_t* screen = ui_screen_create();
    s_title = ui_label_create(screen, &ui_font_semibold_30, "IAQ", 0, 33);
    s_value = ui_centered_label_create(screen, &ui_font_semibold_60, "", 72);
    s_caption = ui_centered_label_create(screen, &ui_font_semibold_15, "Калибровка\nсенсора", 174);
    ui_dots_create(screen, 0);

    s_plate = lv_image_create(screen);
    lv_obj_set_pos(s_plate, 67, 34);
    s_face = lv_image_create(screen);

    return screen;
}

static const level_t* level_of(int iaq)
{
    for (size_t i = 0; i < LEVEL_COUNT - 1; i++) {
        if (iaq <= LEVELS[i].max_iaq) {
            return &LEVELS[i];
        }
    }

    return &LEVELS[LEVEL_COUNT - 1];
}

static void show_calibrating(void)
{
    lv_obj_set_x(s_title, 41);
    lv_obj_add_flag(s_plate, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_value, LV_OBJ_FLAG_HIDDEN);
    lv_image_set_src(s_face, UI_IMAGE("face/ordinary_nimbus"));
    lv_obj_set_pos(s_face, 25, 83);
    lv_obj_remove_flag(s_caption, LV_OBJ_FLAG_HIDDEN);
}

static void show_value(int iaq)
{
    const level_t* level = level_of(iaq);

    lv_obj_set_x(s_title, 13);
    lv_image_set_src(s_plate, level->plate);
    lv_obj_remove_flag(s_plate, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text_fmt(s_value, "%03d", iaq);
    lv_obj_remove_flag(s_value, LV_OBJ_FLAG_HIDDEN);
    lv_image_set_src(s_face, level->face);
    lv_obj_set_pos(s_face, 32, 137);
    lv_obj_add_flag(s_caption, LV_OBJ_FLAG_HIDDEN);
}

void ui_page_iaq_update(const sensing_snapshot_t* snapshot)
{
    // Accuracy 0: BSEC is still stabilizing.
    int shown = SHOWN_CALIBRATING;
    if (snapshot->iaq_accuracy > 0) {
        shown = ui_page_round(snapshot->iaq, 0, IAQ_MAX);
    }

    if (shown == s_shown) {
        return;
    }

    s_shown = shown;
    if (shown == SHOWN_CALIBRATING) {
        show_calibrating();
    } else {
        show_value(shown);
    }
}
