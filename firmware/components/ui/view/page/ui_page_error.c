#include "ui_assets.h"
#include "ui_font.h"
#include "ui_page.h"
#include "ui_widgets.h"

lv_obj_t* ui_page_error_create(void)
{
    lv_obj_t* screen = ui_screen_create();
    ui_image_create(screen, UI_IMAGE("face/dizzy_nimbus"), 26, 45);
    ui_centered_label_create(screen, &ui_font_semibold_15, "Ошибка\nсенсора", 140);

    return screen;
}
