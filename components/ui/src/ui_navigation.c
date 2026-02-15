#include "ui_internal.h"

#include "screens.h"

static const enum ScreensEnum screen_list[] = {
#define X(name) name,
    SCREEN_LIST_X
#undef X
};

static const size_t screen_count = sizeof(screen_list) / sizeof(screen_list[0]);

static int get_current_screen_index(void)
{
    for (size_t i = 0; i < screen_count; i++)
    {
        if (screen_list[i] == currentScreenId)
        {
            return (int)i;
        }
    }

    return 0;
}

void ui_switch_next(void)
{
    int index = get_current_screen_index() + 1;
    if (index >= (int)screen_count)
    {
        index = 0;
    }

    loadScreen(screen_list[index]);
}

void ui_switch_prev(void)
{
    int index = get_current_screen_index() - 1;
    if (index < 0)
    {
        index = (int)screen_count - 1;
    }

    loadScreen(screen_list[index]);
}

void loadScreen(enum ScreensEnum screenId)
{
    if (currentScreenId == screenId) {
        return;
    }

    lv_obj_t* oldScreen = lv_scr_act();

    switch (screenId)
    {
        case SCREEN_ID_IAQ:
            create_screen_iaq();
            break;
        case SCREEN_ID_TEMP:
            create_screen_temp();
            break;
        case SCREEN_ID_HUM:
            create_screen_hum();
            break;
        default:
            break;
    }

    lv_obj_t* newScreen = NULL;
    switch (screenId)
    {
        case SCREEN_ID_IAQ:
            newScreen = ui_objects.screen_iaq;
            break;
        case SCREEN_ID_TEMP:
            newScreen = ui_objects.screen_temp;
            break;
        case SCREEN_ID_HUM:
            newScreen = ui_objects.screen_hum;
            break;
        default:
            break;
    }

    if (newScreen)
    {
        lv_scr_load_anim(newScreen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
        if (oldScreen && oldScreen != newScreen) {
            lv_obj_del(oldScreen);
        }
    }

    currentScreenId = screenId;
    ui_apply_current_values();
}
