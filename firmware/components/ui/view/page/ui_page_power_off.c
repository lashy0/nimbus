#include "ui_assets.h"
#include "ui_font.h"
#include "ui_page.h"
#include "ui_widgets.h"

#define COLOR_ACCENT 0xff6600

#define ANSWERS_Y 200
#define ANSWER_WIDTH 50
#define ANSWER_HEIGHT 28

static lv_obj_t* s_yes;
static lv_obj_t* s_no;

static lv_obj_t* create_answer(lv_obj_t* parent, const char* text, int32_t x)
{
    lv_obj_t* answer = lv_obj_create(parent);
    lv_obj_remove_style_all(answer);
    lv_obj_set_pos(answer, x, ANSWERS_Y);
    lv_obj_set_size(answer, ANSWER_WIDTH, ANSWER_HEIGHT);
    lv_obj_set_style_radius(answer, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(answer, lv_color_hex(COLOR_ACCENT), 0);

    lv_obj_t* label = ui_label_create(answer, ui_font_semibold_15, text, 0, 0);
    lv_obj_center(label);

    return answer;
}

lv_obj_t* ui_page_power_off_create(void)
{
    lv_obj_t* screen = ui_screen_create();
    ui_image_create(screen, UI_IMAGE("face/cat_huh"), 32, 45);
    ui_centered_label_create(screen, ui_font_semibold_15, "Выключить\nустройство?", 140);
    s_yes = create_answer(screen, "ДА", 15);
    s_no = create_answer(screen, "НЕТ", 70);

    return screen;
}

void ui_page_power_off_update(bool yes_selected)
{
    lv_obj_set_style_bg_opa(s_yes, yes_selected ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_opa(s_no, yes_selected ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
}
