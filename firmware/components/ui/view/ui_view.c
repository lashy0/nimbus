#include "ui_view.h"

#define IMAGE_BASE "S:img/misc/base.bin"

void ui_view_create(lv_obj_t* screen)
{
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

    lv_obj_t* face = lv_image_create(screen);
    lv_image_set_src(face, IMAGE_BASE);
    lv_obj_center(face);
}
