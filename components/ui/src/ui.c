#include "ui.h"

#include <stdio.h>
#include "screens.h"
#include "fonts.h"
#include "images.h"

static int current_iaq = 0;
static int current_temp = 0;
static int current_hum = 0;
static int current_batt_pct = 100;
static bool current_batt_charging = false;

static const enum ScreensEnum screen_list[] = {
#define X(name) name,
    SCREEN_LIST_X
#undef X
};

static const size_t screen_count = sizeof(screen_list) / sizeof(screen_list[0]);
static enum ScreensEnum currentScreenId = SCREEN_ID_NONE;

static int get_current_screen_index(void)
{
    for (int i = 0; i < screen_count; i++)
    {
        if (screen_list[i] == currentScreenId)
        {
            return (int)i;
        }
    }

    return 0;
}

static void ui_apply_current_values(void)
{
    char buf[16];
    
    switch (currentScreenId)
    {
        case SCREEN_ID_IAQ:
            if (ui_objects.lbl_iaq_value) {
                snprintf(buf, sizeof(buf), "%03d", current_iaq);
                lv_label_set_text(ui_objects.lbl_iaq_value, buf);
            }
            if (ui_objects.img_iaq_icon) {
                img_set_info(ui_objects.img_iaq_icon, get_iaq_info(current_iaq));
            }
            if (ui_objects.img_iaq_status) {
                img_set_info(ui_objects.img_iaq_status, get_iaq_status_info(current_iaq));
            }
            if (ui_objects.img_iaq_battery) {
                img_set_info(ui_objects.img_iaq_battery, get_battery_info(current_batt_pct, current_batt_charging));
            }
            if (ui_objects.lbl_iaq_batt_pct) {
                snprintf(buf, sizeof(buf), "%d %%", current_batt_pct);
                lv_label_set_text(ui_objects.lbl_iaq_batt_pct, buf);
            }
            break;
            
        case SCREEN_ID_TEMP:
            if (ui_objects.lbl_temp_value) {
                snprintf(buf, sizeof(buf), "%d°", current_temp);
                lv_label_set_text(ui_objects.lbl_temp_value, buf);
            }
            if (ui_objects.img_temp_icon) {
                img_set_info(ui_objects.img_temp_icon, get_temp_info(current_temp));
            }
            if (ui_objects.img_temp_status) {
                img_set_info(ui_objects.img_temp_status, get_temp_status_info(current_temp));
            }
            if (ui_objects.img_temp_battery) {
                img_set_info(ui_objects.img_temp_battery, get_battery_info(current_batt_pct, current_batt_charging));
            }
            if (ui_objects.lbl_temp_batt_pct) {
                snprintf(buf, sizeof(buf), "%d %%", current_batt_pct);
                lv_label_set_text(ui_objects.lbl_temp_batt_pct, buf);
            }
            break;
            
        case SCREEN_ID_HUM:
            if (ui_objects.lbl_hum_value) {
                snprintf(buf, sizeof(buf), "%d%%", current_hum);
                lv_label_set_text(ui_objects.lbl_hum_value, buf);

                if (current_hum  == 100) {
                    lv_obj_set_style_text_font(ui_objects.lbl_hum_value, 
                        &ui_font_sf_sb_50_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
                } else {
                    lv_obj_set_style_text_font(ui_objects.lbl_hum_value, 
                        &ui_font_sf_sb_60_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
                }
            }
            if (ui_objects.img_hum_icon) {
                img_set_info(ui_objects.img_hum_icon, get_hum_info(current_hum));
            }
            if (ui_objects.img_hum_status) {
                img_set_info(ui_objects.img_hum_status, get_hum_status_info(current_hum));
            }
            if (ui_objects.img_hum_battery) {
                img_set_info(ui_objects.img_hum_battery, get_battery_info(current_batt_pct, current_batt_charging));
            }
            if (ui_objects.lbl_hum_batt_pct) {
                snprintf(buf, sizeof(buf), "%d %%", current_batt_pct);
                lv_label_set_text(ui_objects.lbl_hum_batt_pct, buf);
            }
            break;
            
        default:
            break;
    }
}

void ui_switch_next(void)
{
    int index = get_current_screen_index();
    index++;

    if (index >= screen_count)
    {
        index = 0;
    }

    loadScreen(screen_list[index]);
}

void ui_switch_prev(void)
{
    int index = get_current_screen_index();
    index--;

    if (index < 0)
    {
        index = screen_count - 1;
    }

    loadScreen(screen_list[index]);
}

void loadScreen(enum ScreensEnum screenId)
{
    if (currentScreenId == screenId) return;

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

void ui_init() { loadScreen(SCREEN_ID_IAQ); }

void ui_tick() {}

void ui_update_iaq(int value)
{
    current_iaq = value;
    
    if (currentScreenId == SCREEN_ID_IAQ)
    {
        char buf[8];
        if (ui_objects.lbl_iaq_value) {
            snprintf(buf, sizeof(buf), "%03d", value);
            lv_label_set_text(ui_objects.lbl_iaq_value, buf);
        }
        if (ui_objects.img_iaq_icon) {
            img_set_info(ui_objects.img_iaq_icon, get_iaq_info(value));
        }
        if (ui_objects.img_iaq_status) {
            img_set_info(ui_objects.img_iaq_status, get_iaq_status_info(value));
        }
    }
}

void ui_update_temp(int value)
{
    current_temp = value;
    
    if (currentScreenId == SCREEN_ID_TEMP)
    {
        char buf[8];
        if (ui_objects.lbl_temp_value) {
            snprintf(buf, sizeof(buf), "%d°", value);
            lv_label_set_text(ui_objects.lbl_temp_value, buf);
        }
        if (ui_objects.img_temp_icon) {
            img_set_info(ui_objects.img_temp_icon, get_temp_info(value));
        }
        if (ui_objects.img_temp_status) {
            img_set_info(ui_objects.img_temp_status, get_temp_status_info(value));
        }
    }
}

void ui_update_hum(int value)
{
    current_hum = value;
    
    if (currentScreenId == SCREEN_ID_HUM)
    {
        char buf[8];
        if (ui_objects.lbl_hum_value) {
            snprintf(buf, sizeof(buf), "%d%%", value);
            lv_label_set_text(ui_objects.lbl_hum_value, buf);

            if (value == 100) {
                lv_obj_set_style_text_font(ui_objects.lbl_hum_value, 
                    &ui_font_sf_sb_50_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
            } else {
                lv_obj_set_style_text_font(ui_objects.lbl_hum_value, 
                    &ui_font_sf_sb_60_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
        }
        if (ui_objects.img_hum_icon) {
            img_set_info(ui_objects.img_hum_icon, get_hum_info(value));
        }
        if (ui_objects.img_hum_status) {
            img_set_info(ui_objects.img_hum_status, get_hum_status_info(value));
        }
    }
}

void ui_update_battery(int percent, bool charging)
{
    current_batt_pct = percent;
    current_batt_charging = charging;
    
    char buf[8];
    snprintf(buf, sizeof(buf), "%d %%", percent);
    const img_info_t* icon = get_battery_info(percent, charging);

    switch (currentScreenId)
    {
        case SCREEN_ID_IAQ:
            if (ui_objects.img_iaq_battery) {
                img_set_info(ui_objects.img_iaq_battery, icon);
            }
            if (ui_objects.lbl_iaq_batt_pct) {
                lv_label_set_text(ui_objects.lbl_iaq_batt_pct, buf);
            }
            break;
            
        case SCREEN_ID_TEMP:
            if (ui_objects.img_temp_battery) {
                img_set_info(ui_objects.img_temp_battery, icon);
            }
            if (ui_objects.lbl_temp_batt_pct) {
                lv_label_set_text(ui_objects.lbl_temp_batt_pct, buf);
            }
            break;
            
        case SCREEN_ID_HUM:
            if (ui_objects.img_hum_battery) {
                img_set_info(ui_objects.img_hum_battery, icon);
            }
            if (ui_objects.lbl_hum_batt_pct) {
                lv_label_set_text(ui_objects.lbl_hum_batt_pct, buf);
            }
            break;
            
        default:
            break;
    }
}