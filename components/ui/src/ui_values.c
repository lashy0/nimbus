#include "ui_internal.h"

#include <stdio.h>

#include "fonts.h"
#include "images.h"
#include "screens.h"

void ui_apply_brightness_value(void)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%u%%", (unsigned int)current_brightness_pct);

    if (ui_objects.lbl_brightness_value) {
        lv_obj_set_style_text_font(ui_objects.lbl_brightness_value, &ui_font_sf_sb_30_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(ui_objects.lbl_brightness_value, buf);
    }

    if (ui_objects.bar_brightness) {
        lv_bar_set_value(ui_objects.bar_brightness, current_brightness_pct, LV_ANIM_OFF);
    }
}

static void ui_apply_iaq_screen_state(void)
{
    bool warmup = (current_iaq_accuracy == 0U);
    char buf[8];

    if (warmup) {
        if (ui_objects.lbl_iaq_title) {
            lv_obj_set_pos(ui_objects.lbl_iaq_title, 41, 33);
            lv_obj_set_size(ui_objects.lbl_iaq_title, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        }
        if (ui_objects.lbl_iaq_value) {
            lv_obj_add_flag(ui_objects.lbl_iaq_value, LV_OBJ_FLAG_HIDDEN);
        }
        if (ui_objects.img_iaq_status) {
            lv_obj_add_flag(ui_objects.img_iaq_status, LV_OBJ_FLAG_HIDDEN);
        }
        if (ui_objects.img_iaq_icon) {
            img_set(ui_objects.img_iaq_icon, &IMG_INFO_ORDINARY_NIMBUS);
        }
        if (ui_objects.lbl_iaq_warmup) {
            lv_obj_clear_flag(ui_objects.lbl_iaq_warmup, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    if (ui_objects.lbl_iaq_title) {
        lv_obj_set_pos(ui_objects.lbl_iaq_title, 13, 34);
        lv_obj_set_size(ui_objects.lbl_iaq_title, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    }

    if (ui_objects.lbl_iaq_value) {
        snprintf(buf, sizeof(buf), "%03d", current_iaq);
        lv_label_set_text(ui_objects.lbl_iaq_value, buf);
        lv_obj_clear_flag(ui_objects.lbl_iaq_value, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_objects.img_iaq_icon) {
        img_set_info(ui_objects.img_iaq_icon, get_iaq_info(current_iaq));
    }
    if (ui_objects.img_iaq_status) {
        img_set_info(ui_objects.img_iaq_status, get_iaq_status_info(current_iaq));
        lv_obj_clear_flag(ui_objects.img_iaq_status, LV_OBJ_FLAG_HIDDEN);
    }
    if (ui_objects.lbl_iaq_warmup) {
        lv_obj_add_flag(ui_objects.lbl_iaq_warmup, LV_OBJ_FLAG_HIDDEN);
    }
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

static void ui_get_current_battery_widgets(lv_obj_t** img_obj, lv_obj_t** label_obj)
{
    if (!img_obj || !label_obj) {
        return;
    }

    *img_obj = NULL;
    *label_obj = NULL;

    switch (currentScreenId) {
        case SCREEN_ID_IAQ:
            *img_obj = ui_objects.img_iaq_battery;
            *label_obj = ui_objects.lbl_iaq_batt_pct;
            break;
        case SCREEN_ID_TEMP:
            *img_obj = ui_objects.img_temp_battery;
            *label_obj = ui_objects.lbl_temp_batt_pct;
            break;
        case SCREEN_ID_HUM:
            *img_obj = ui_objects.img_hum_battery;
            *label_obj = ui_objects.lbl_hum_batt_pct;
            break;
        case SCREEN_ID_QUESTION:
            *img_obj = ui_objects.img_question_battery;
            *label_obj = ui_objects.lbl_question_batt_pct;
            break;
        case SCREEN_ID_BRIGHTNESS:
            *img_obj = ui_objects.img_brightness_battery;
            *label_obj = ui_objects.lbl_brightness_batt_pct;
            break;
        default:
            break;
    }
}

static void ui_apply_battery_widgets(lv_obj_t* img_obj, lv_obj_t* label_obj)
{
    char buf[16];

    if (ui_battery_is_known() && img_obj) {
        img_set_info(img_obj, get_battery_info(current_batt_pct, current_batt_charging));
    }
    if (label_obj) {
        lv_label_set_text(label_obj, ui_battery_text(buf, sizeof(buf)));
    }
}

void ui_apply_current_battery_status(void)
{
    lv_obj_t* img_obj = NULL;
    lv_obj_t* label_obj = NULL;
    ui_get_current_battery_widgets(&img_obj, &label_obj);
    ui_apply_battery_widgets(img_obj, label_obj);
}

void ui_apply_current_values(void)
{
    char buf[16];

    switch (currentScreenId) {
        case SCREEN_ID_IAQ:
            ui_apply_iaq_screen_state();
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
            break;

        case SCREEN_ID_HUM:
            if (ui_objects.lbl_hum_value) {
                snprintf(buf, sizeof(buf), "%d%%", current_hum);
                lv_label_set_text(ui_objects.lbl_hum_value, buf);

                const lv_font_t* font = (current_hum == 100) ? &ui_font_sf_sb_50_digits : &ui_font_sf_sb_60_digits;
                lv_obj_set_style_text_font(ui_objects.lbl_hum_value, font, LV_PART_MAIN | LV_STATE_DEFAULT);
            }
            if (ui_objects.img_hum_icon) {
                img_set_info(ui_objects.img_hum_icon, get_hum_info(current_hum));
            }
            if (ui_objects.img_hum_status) {
                img_set_info(ui_objects.img_hum_status, get_hum_status_info(current_hum));
            }
            break;

        case SCREEN_ID_BRIGHTNESS:
            ui_apply_brightness_value();
            break;

        case SCREEN_ID_QUESTION:
        default:
            break;
    }

    ui_apply_current_battery_status();
}

void ui_update_iaq(int value)
{
    current_iaq = value;

    if (currentScreenId == SCREEN_ID_IAQ) {
        ui_apply_iaq_screen_state();
    }
}

void ui_update_temp(int value)
{
    current_temp = value;

    if (currentScreenId == SCREEN_ID_TEMP) {
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

    if (currentScreenId == SCREEN_ID_HUM) {
        char buf[8];
        if (ui_objects.lbl_hum_value) {
            snprintf(buf, sizeof(buf), "%d%%", value);
            lv_label_set_text(ui_objects.lbl_hum_value, buf);

            const lv_font_t* font = (value == 100) ? &ui_font_sf_sb_50_digits : &ui_font_sf_sb_60_digits;
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

    if (currentScreenId == SCREEN_ID_IAQ) {
        ui_apply_iaq_screen_state();
    }
}

void ui_update_iaq_quality(uint8_t iaq_accuracy, bool stabilization_done, bool run_in_done)
{
    current_iaq_accuracy = iaq_accuracy;
    current_stabilization_done = stabilization_done;
    current_run_in_done = run_in_done;

    if (currentScreenId == SCREEN_ID_IAQ) {
        ui_apply_iaq_screen_state();
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

    ui_apply_current_battery_status();
}
