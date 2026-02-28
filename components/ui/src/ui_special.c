#include "ui_internal.h"

#include "screens.h"

void ui_show_start(void)
{
    lv_obj_t* oldScreen = lv_scr_act();
    create_screen_start();

    lv_scr_load(ui_objects.screen_start);
    if (oldScreen) {
        lv_obj_del(oldScreen);
    }
    currentScreenId = SCREEN_ID_START;
}

void ui_show_charging(void)
{
    previousScreenId = currentScreenId;

    lv_obj_t* oldScreen = lv_scr_act();
    create_screen_charging();

    lv_scr_load(ui_objects.screen_charging);
    if (oldScreen) {
        lv_obj_del(oldScreen);
    }
    currentScreenId = SCREEN_ID_CHARGING;
    ui_apply_current_values();
}

void ui_show_no_charging(void)
{
    previousScreenId = currentScreenId;

    lv_obj_t* oldScreen = lv_scr_act();
    create_screen_no_charging();

    lv_scr_load(ui_objects.screen_no_charging);
    if (oldScreen) {
        lv_obj_del(oldScreen);
    }
    currentScreenId = SCREEN_ID_NO_CHARGING;
    ui_apply_current_values();
}

void ui_show_calibration(void)
{
    previousScreenId = currentScreenId;

    lv_obj_t* oldScreen = lv_scr_act();
    create_screen_calibration();

    lv_scr_load(ui_objects.screen_calibration);
    if (oldScreen) {
        lv_obj_del(oldScreen);
    }
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
    if (oldScreen) {
        lv_obj_del(oldScreen);
    }
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
    if (previousScreenId != SCREEN_ID_NONE && previousScreenId != SCREEN_ID_START &&
        previousScreenId != SCREEN_ID_CHARGING && previousScreenId != SCREEN_ID_NO_CHARGING &&
        previousScreenId != SCREEN_ID_CALIBRATION && previousScreenId != SCREEN_ID_BRIGHTNESS &&
        previousScreenId != SCREEN_ID_QUESTION) {
        loadScreen(previousScreenId);
    } else {
        loadScreen(SCREEN_ID_IAQ);
    }
}
