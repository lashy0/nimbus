#include "ui_assets.h"
#include "ui_font.h"
#include "ui_page.h"
#include "ui_widgets.h"

#define COLOR_BAR_TRACK 0x9a9a9a

static lv_obj_t* s_caption;
static lv_obj_t* s_bar;
static int s_shown = -1; // Percent on screen.

static lv_obj_t* create_bar(lv_obj_t* parent)
{
    lv_obj_t* bar = lv_bar_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_pos(bar, 10, 181);
    lv_obj_set_size(bar, 115, 5);
    lv_bar_set_range(bar, 0, 100);

    lv_obj_set_style_radius(bar, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(COLOR_BAR_TRACK), LV_PART_MAIN);
    lv_obj_set_style_radius(bar, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar, lv_color_white(), LV_PART_INDICATOR);

    return bar;
}

lv_obj_t* ui_page_brightness_create(void)
{
    lv_obj_t* screen = ui_screen_create();
    ui_image_create(screen, UI_IMAGE("misc/sun"), 40, 60);
    s_caption = ui_centered_label_create(screen, &ui_font_semibold_15, "", 125);
    s_bar = create_bar(screen);

    return screen;
}

void ui_page_brightness_update(uint8_t percent)
{
    if (percent == s_shown) {
        return;
    }

    s_shown = percent;
    lv_label_set_text_fmt(s_caption, "Яркость\n%u%%", percent);
    lv_bar_set_value(s_bar, percent, LV_ANIM_OFF);
}
