#include "ui_assets.h"
#include "ui_page.h"
#include "ui_widgets.h"

lv_obj_t* ui_page_usb_create(void)
{
    lv_obj_t* screen = ui_screen_create();
    ui_image_create(screen, UI_IMAGE("misc/lightning_charge"), 38, 70);

    return screen;
}
