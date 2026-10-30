#include "ui_widgets.h"

#define COLOR_INACTIVE_DOT 0x808080

#define DOT_COUNT 3
#define DOT_SIZE 5
#define DOT_STEP 15
#define DOTS_X 50
#define DOTS_Y 225

lv_obj_t* ui_screen_create(void)
{
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_text_color(screen, lv_color_white(), 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    return screen;
}

lv_obj_t* ui_label_create(lv_obj_t* parent, const lv_font_t* font, const char* text, int32_t x, int32_t y)
{
    lv_obj_t* label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);

    return label;
}

lv_obj_t* ui_centered_label_create(lv_obj_t* parent, const lv_font_t* font, const char* text, int32_t y)
{
    lv_obj_t* label = ui_label_create(parent, font, text, 0, y);
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

    return label;
}

lv_obj_t* ui_image_create(lv_obj_t* parent, const char* src, int32_t x, int32_t y)
{
    lv_obj_t* image = lv_image_create(parent);
    lv_image_set_src(image, src);
    lv_obj_set_pos(image, x, y);

    return image;
}

void ui_dots_create(lv_obj_t* parent, uint32_t active)
{
    for (uint32_t i = 0; i < DOT_COUNT; i++) {
        lv_obj_t* dot = lv_obj_create(parent);
        lv_obj_remove_style_all(dot);
        lv_obj_set_pos(dot, DOTS_X + DOT_STEP * i, DOTS_Y);
        lv_obj_set_size(dot, DOT_SIZE, DOT_SIZE);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(dot, i == active ? lv_color_white() : lv_color_hex(COLOR_INACTIVE_DOT), 0);
    }
}
