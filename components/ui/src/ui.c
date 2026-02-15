#include "ui.h"

#include <stdio.h>
#include "screens.h"
#include "fonts.h"
#include "images.h"

static int current_iaq = 0;
static int current_temp = 0;
static int current_hum = 0;
static int current_batt_pct = -1;
static bool current_batt_charging = false;
static bool current_stabilization_done = false;
static bool current_run_in_done = false;
static uint8_t current_brightness_pct = 60;
static const uint8_t UI_BRIGHTNESS_MIN_PCT = 5;

static const enum ScreensEnum screen_list[] = {
#define X(name) name,
    SCREEN_LIST_X
#undef X
};

static const size_t screen_count = sizeof(screen_list) / sizeof(screen_list[0]);
static enum ScreensEnum currentScreenId = SCREEN_ID_NONE;
static enum ScreensEnum previousScreenId = SCREEN_ID_NONE;
static bool startup_non_critical_error = false;
static lv_obj_t* startup_error_icon = NULL;

static void (*question_on_yes)(void) = NULL;
static void (*question_on_no)(void) = NULL;
static bool question_selected_yes = true;

static void ui_apply_brightness_value(void)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%u%%", (unsigned int)current_brightness_pct);

    if (ui_objects.lbl_brightness_value) {
        const lv_font_t* font = (current_brightness_pct == 100)
            ? &ui_font_sf_sb_50_digits
            : &ui_font_sf_sb_60_digits;
        lv_obj_set_style_text_font(ui_objects.lbl_brightness_value, font, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    if (ui_objects.lbl_brightness_value) {
        lv_label_set_text(ui_objects.lbl_brightness_value, buf);
    }

    if (ui_objects.bar_brightness) {
        lv_bar_set_value(ui_objects.bar_brightness, current_brightness_pct, LV_ANIM_OFF);
    }
}

static void ui_apply_calibration_status_text(void)
{
    if (!ui_objects.lbl_calibration_text) {
        return;
    }

    char buf[96];
    snprintf(buf,
             sizeof(buf),
             "Calibrating...\nStab:%s Run:%s",
             current_stabilization_done ? "ok" : "wait",
             current_run_in_done ? "ok" : "wait");
    lv_label_set_text(ui_objects.lbl_calibration_text, buf);
}

static void ui_show_startup_error_indicator(void)
{
    if (!startup_non_critical_error || startup_error_icon) {
        return;
    }

    startup_error_icon = lv_img_create(lv_layer_top());
    img_set(startup_error_icon, &IMG_INFO_DEAD);
    lv_obj_set_pos(startup_error_icon, 2, 2);
    lv_obj_clear_flag(startup_error_icon, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
}

enum ScreensEnum ui_get_current_screen(void)
{
    return currentScreenId;
}

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

static bool ui_battery_is_known(void)
{
    return current_batt_pct >= 0;
}

static const char* ui_battery_text(char* buf, size_t buf_size)
{
    if (!ui_battery_is_known()) {
        return "-- %";
    }

    snprintf(buf, buf_size, "%d %%", current_batt_pct);
    return buf;
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
                if (ui_battery_is_known()) {
                    img_set_info(ui_objects.img_iaq_battery, get_battery_info(current_batt_pct, current_batt_charging));
                }
            }
            if (ui_objects.lbl_iaq_batt_pct) {
                lv_label_set_text(ui_objects.lbl_iaq_batt_pct, ui_battery_text(buf, sizeof(buf)));
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
                if (ui_battery_is_known()) {
                    img_set_info(ui_objects.img_temp_battery, get_battery_info(current_batt_pct, current_batt_charging));
                }
            }
            if (ui_objects.lbl_temp_batt_pct) {
                lv_label_set_text(ui_objects.lbl_temp_batt_pct, ui_battery_text(buf, sizeof(buf)));
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
                if (ui_battery_is_known()) {
                    img_set_info(ui_objects.img_hum_battery, get_battery_info(current_batt_pct, current_batt_charging));
                }
            }
            if (ui_objects.lbl_hum_batt_pct) {
                lv_label_set_text(ui_objects.lbl_hum_batt_pct, ui_battery_text(buf, sizeof(buf)));
            }
            break;
        
        case SCREEN_ID_CALIBRATION:
            if (ui_objects.img_calibration_battery) {
                if (ui_battery_is_known()) {
                    img_set_info(ui_objects.img_calibration_battery, get_battery_info(current_batt_pct, current_batt_charging));
                }
            }
            if (ui_objects.lbl_calibration_batt_pct) {
                lv_label_set_text(ui_objects.lbl_calibration_batt_pct, ui_battery_text(buf, sizeof(buf)));
            }
            ui_apply_calibration_status_text();
            break;
        
        case SCREEN_ID_QUESTION:
            if (ui_objects.img_question_battery) {
                if (ui_battery_is_known()) {
                    img_set_info(ui_objects.img_question_battery, get_battery_info(current_batt_pct, current_batt_charging));
                }
            }
            if (ui_objects.lbl_question_batt_pct) {
                lv_label_set_text(ui_objects.lbl_question_batt_pct, ui_battery_text(buf, sizeof(buf)));
            }
            break;

        case SCREEN_ID_BRIGHTNESS:
            if (ui_objects.img_brightness_battery) {
                if (ui_battery_is_known()) {
                    img_set_info(ui_objects.img_brightness_battery, get_battery_info(current_batt_pct, current_batt_charging));
                }
            }
            if (ui_objects.lbl_brightness_batt_pct) {
                lv_label_set_text(ui_objects.lbl_brightness_batt_pct, ui_battery_text(buf, sizeof(buf)));
            }
            ui_apply_brightness_value();
            break;
            
        default:
            break;
    }
}

void ui_switch_next(void)
{
    int index = get_current_screen_index();
    index++;

    if (index >= (int)screen_count)
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

void ui_init() { ui_show_start(); }

void ui_finish_startup(bool has_non_critical_error)
{
    startup_non_critical_error = has_non_critical_error;

    if (startup_non_critical_error) {
        ui_show_no_charging();
    } else if (currentScreenId == SCREEN_ID_START) {
        loadScreen(SCREEN_ID_IAQ);
    }

    ui_show_startup_error_indicator();
}

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

            const lv_font_t* font = (value == 100)
                ? &ui_font_sf_sb_50_digits
                : &ui_font_sf_sb_60_digits;
            lv_obj_set_style_text_font(ui_objects.lbl_hum_value, font, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (ui_objects.img_hum_icon) {
            img_set_info(ui_objects.img_hum_icon, get_hum_info(value));
        }
        if (ui_objects.img_hum_status) {
            img_set_info(ui_objects.img_hum_status, get_hum_status_info(value));
        }
    }
}

void ui_update_calibration_status(bool stabilization_done, bool run_in_done)
{
    current_stabilization_done = stabilization_done;
    current_run_in_done = run_in_done;

    if (currentScreenId == SCREEN_ID_CALIBRATION) {
        ui_apply_calibration_status_text();
    }
}

void ui_update_battery(int percent, bool charging)
{
    if (percent < 0) {
        current_batt_pct = -1;
        current_batt_charging = false;
    } else {
        if (percent > 100) {
            percent = 100;
        }
        current_batt_pct = percent;
        current_batt_charging = charging;
    }
    
    char buf[8];
    const char* batt_text = ui_battery_text(buf, sizeof(buf));
    const img_info_t* icon = ui_battery_is_known() ? get_battery_info(current_batt_pct, current_batt_charging) : NULL;

    switch (currentScreenId)
    {
        case SCREEN_ID_IAQ:
            if (icon && ui_objects.img_iaq_battery) {
                img_set_info(ui_objects.img_iaq_battery, icon);
            }
            if (ui_objects.lbl_iaq_batt_pct) {
                lv_label_set_text(ui_objects.lbl_iaq_batt_pct, batt_text);
            }
            break;
            
        case SCREEN_ID_TEMP:
            if (icon && ui_objects.img_temp_battery) {
                img_set_info(ui_objects.img_temp_battery, icon);
            }
            if (ui_objects.lbl_temp_batt_pct) {
                lv_label_set_text(ui_objects.lbl_temp_batt_pct, batt_text);
            }
            break;
            
        case SCREEN_ID_HUM:
            if (icon && ui_objects.img_hum_battery) {
                img_set_info(ui_objects.img_hum_battery, icon);
            }
            if (ui_objects.lbl_hum_batt_pct) {
                lv_label_set_text(ui_objects.lbl_hum_batt_pct, batt_text);
            }
            break;
        
        case SCREEN_ID_CALIBRATION:
            if (icon && ui_objects.img_calibration_battery) {
                img_set_info(ui_objects.img_calibration_battery, icon);
            }
            if (ui_objects.lbl_calibration_batt_pct) {
                lv_label_set_text(ui_objects.lbl_calibration_batt_pct, batt_text);
            }
            break;
        
        case SCREEN_ID_QUESTION:
            if (icon && ui_objects.img_question_battery) {
                img_set_info(ui_objects.img_question_battery, icon);
            }
            if (ui_objects.lbl_question_batt_pct) {
                lv_label_set_text(ui_objects.lbl_question_batt_pct, batt_text);
            }
            break;

        case SCREEN_ID_BRIGHTNESS:
            if (icon && ui_objects.img_brightness_battery) {
                img_set_info(ui_objects.img_brightness_battery, icon);
            }
            if (ui_objects.lbl_brightness_batt_pct) {
                lv_label_set_text(ui_objects.lbl_brightness_batt_pct, batt_text);
            }
            break;
            
        default:
            break;
    }
}

void ui_show_start(void)
{
    lv_obj_t* oldScreen = lv_scr_act();
    create_screen_start();

    lv_scr_load(ui_objects.screen_start);
    if (oldScreen) lv_obj_del(oldScreen);
    currentScreenId = SCREEN_ID_START;
}

void ui_show_charging(void)
{
    previousScreenId = currentScreenId;

    lv_obj_t* oldScreen = lv_scr_act();
    create_screen_charging();

    lv_scr_load(ui_objects.screen_charging);
    if (oldScreen) lv_obj_del(oldScreen);
    currentScreenId = SCREEN_ID_CHARGING;
}

void ui_show_no_charging(void)
{
    previousScreenId = currentScreenId;

    lv_obj_t* oldScreen = lv_scr_act();
    create_screen_no_charging();

    lv_scr_load(ui_objects.screen_no_charging);
    if (oldScreen) lv_obj_del(oldScreen);
    currentScreenId = SCREEN_ID_NO_CHARGING;
}

void ui_show_calibration(void)
{
    previousScreenId = currentScreenId;

    lv_obj_t* oldScreen = lv_scr_act();
    create_screen_calibration();

    lv_scr_load(ui_objects.screen_calibration);
    if (oldScreen) lv_obj_del(oldScreen);
    currentScreenId = SCREEN_ID_CALIBRATION;
    ui_apply_current_values();
}

void ui_show_brightness(uint8_t value_percent)
{
    if (value_percent < UI_BRIGHTNESS_MIN_PCT) {
        value_percent = UI_BRIGHTNESS_MIN_PCT;
    } else if (value_percent > 100) {
        value_percent = 100;
    }

    current_brightness_pct = value_percent;
    previousScreenId = currentScreenId;

    lv_obj_t* oldScreen = lv_scr_act();
    create_screen_brightness(current_brightness_pct);

    lv_scr_load(ui_objects.screen_brightness);
    if (oldScreen) {
        lv_obj_del(oldScreen);
    }
    currentScreenId = SCREEN_ID_BRIGHTNESS;
    ui_apply_current_values();
}

void ui_update_brightness_value(uint8_t value_percent)
{
    if (value_percent < UI_BRIGHTNESS_MIN_PCT) {
        value_percent = UI_BRIGHTNESS_MIN_PCT;
    } else if (value_percent > 100) {
        value_percent = 100;
    }

    current_brightness_pct = value_percent;
    if (currentScreenId == SCREEN_ID_BRIGHTNESS) {
        ui_apply_brightness_value();
    }
}

void ui_show_question(const char* text, void (*on_yes)(void), void (*on_no)(void), bool select_yes)
{
    previousScreenId = currentScreenId;
    question_on_yes = on_yes;
    question_on_no = on_no;
    question_selected_yes = select_yes;

    lv_obj_t* oldScreen = lv_scr_act();
    create_screen_question(text);
    lv_scr_load(ui_objects.screen_question);
    if (oldScreen) lv_obj_del(oldScreen);
    currentScreenId = SCREEN_ID_QUESTION;

    if (select_yes) {
        ui_question_select_yes();
    } else {
        ui_question_select_no();
    }

    ui_apply_current_values();
}

void ui_question_select_yes(void)
{
    question_selected_yes = true;
    if (ui_objects.btn_question_yes) {
        lv_obj_set_style_bg_color(ui_objects.btn_question_yes, lv_color_hex(0xffff6600), LV_PART_MAIN);
    }
    if (ui_objects.btn_question_no) {
        lv_obj_set_style_bg_color(ui_objects.btn_question_no, lv_color_hex(0xff333333), LV_PART_MAIN);
    }
}

void ui_question_select_no(void)
{
    question_selected_yes = false;
    if (ui_objects.btn_question_yes) {
        lv_obj_set_style_bg_color(ui_objects.btn_question_yes, lv_color_hex(0xff333333), LV_PART_MAIN);
    }
    if (ui_objects.btn_question_no) {
        lv_obj_set_style_bg_color(ui_objects.btn_question_no, lv_color_hex(0xffff6600), LV_PART_MAIN);
    }
}

void ui_question_confirm(void)
{
    void (*callback)(void) = question_selected_yes ? question_on_yes : question_on_no;

    question_on_yes = NULL;
    question_on_no = NULL;

    if (callback) {
        callback();
    }
}

void ui_hide_special(void)
{
    if (previousScreenId != SCREEN_ID_NONE &&
        previousScreenId != SCREEN_ID_START &&
        previousScreenId != SCREEN_ID_CHARGING &&
        previousScreenId != SCREEN_ID_NO_CHARGING &&
        previousScreenId != SCREEN_ID_CALIBRATION &&
        previousScreenId != SCREEN_ID_BRIGHTNESS &&
        previousScreenId != SCREEN_ID_QUESTION)
    {
        loadScreen(previousScreenId);
    }
    else
    {
        loadScreen(SCREEN_ID_IAQ);
    }
}
