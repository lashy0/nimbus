#include "ui_assets.h"
#include "ui_page.h"
#include "ui_widgets.h"

lv_obj_t* ui_page_start_create(void)
{
    lv_obj_t* screen = ui_screen_create();

    lv_obj_t* face = lv_image_create(screen);
    lv_image_set_src(face, UI_IMAGE("misc/base"));
    lv_obj_center(face);

    return screen;
}
