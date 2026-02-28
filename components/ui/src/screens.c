#include "screens.h"

#include <stdio.h>
#include "fonts.h"
#include "images.h"
#include "ui.h"

ui_objects_t ui_objects;

static void create_status_bar(lv_obj_t* parent, lv_obj_t** out_img_batt, lv_obj_t** out_lbl_pct)
{
    lv_obj_t* img = lv_img_create(parent);
    *out_img_batt = img;
    img_set(img, &IMG_INFO_BATT_FULL_NOT_CHARGING);

    lv_obj_t* lbl = lv_label_create(parent);
    *out_lbl_pct = lbl;
    lv_obj_set_pos(lbl, 41, 12);
    lv_obj_set_size(lbl, 30, 8);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), 0);
    lv_obj_set_style_text_font(lbl, &ui_font_sf_b_10_digits, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(lbl, "00 %");
}

static void create_page_indicators(lv_obj_t* parent, int active_index)
{
    int x_positions[] = {50, 65, 80};
    int y_pos = 225;

    for (int i = 0; i < 3; i++) {
        lv_obj_t* obj = lv_obj_create(parent);

        lv_obj_set_pos(obj, x_positions[i], y_pos);
        lv_obj_set_size(obj, 5, 5);
        lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

        lv_color_t color = (i == active_index) ? lv_color_hex(0xffffffff) : lv_color_hex(0xff808080);

        lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void create_screen_iaq(void)
{
    lv_obj_t* obj = lv_obj_create(0);
    ui_objects.screen_iaq = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 135, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    create_status_bar(obj, &ui_objects.img_iaq_battery, &ui_objects.lbl_iaq_batt_pct);

    // Title Label IAQ
    ui_objects.lbl_iaq_title = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_iaq_title, 13, 34);
    lv_obj_set_size(ui_objects.lbl_iaq_title, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(ui_objects.lbl_iaq_title, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_objects.lbl_iaq_title, &ui_font_sf_sb_30_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_objects.lbl_iaq_title, "IAQ");

    // Status Img
    ui_objects.img_iaq_status = lv_img_create(obj);
    img_set(ui_objects.img_iaq_status, &IMG_INFO_GOOD);

    // Value Label
    ui_objects.lbl_iaq_value = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_iaq_value, 0, 72);
    lv_obj_set_size(ui_objects.lbl_iaq_value, 135, 49);
    lv_obj_set_style_text_color(ui_objects.lbl_iaq_value, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_objects.lbl_iaq_value, &ui_font_sf_sb_60_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_objects.lbl_iaq_value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_objects.lbl_iaq_value, "000");

    ui_objects.lbl_iaq_cal_status = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_iaq_cal_status, 0, 124);
    lv_obj_set_size(ui_objects.lbl_iaq_cal_status, 135, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(ui_objects.lbl_iaq_cal_status, lv_color_hex(0xffa0a0a0), 0);
    lv_obj_set_style_text_font(ui_objects.lbl_iaq_cal_status, &ui_font_sf_b_10_digits, 0);
    lv_obj_set_style_text_align(ui_objects.lbl_iaq_cal_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(ui_objects.lbl_iaq_cal_status, "WARMUP ACC 0/3");

    // Icon Img
    ui_objects.img_iaq_icon = lv_img_create(obj);
    img_set(ui_objects.img_iaq_icon, &IMG_INFO_ULTRA_HAPPY);

    create_page_indicators(obj, 0);

    tick_screen_iaq();
}

void tick_screen_iaq() {}

void create_screen_temp(void)
{
    lv_obj_t* obj = lv_obj_create(0);
    ui_objects.screen_temp = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 135, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    create_status_bar(obj, &ui_objects.img_temp_battery, &ui_objects.lbl_temp_batt_pct);

    // Title Label Temp
    ui_objects.lbl_temp_title = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_temp_title, 20, 34);
    lv_obj_set_size(ui_objects.lbl_temp_title, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(ui_objects.lbl_temp_title, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_objects.lbl_temp_title, &ui_font_sf_sb_30_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_objects.lbl_temp_title, "Temp");

    // Status Img
    ui_objects.img_temp_status = lv_img_create(obj);
    img_set(ui_objects.img_temp_status, &IMG_INFO_NOTHING);

    // Value Label
    ui_objects.lbl_temp_value = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_temp_value, 0, 72);
    lv_obj_set_size(ui_objects.lbl_temp_value, 135, 49);
    lv_obj_set_style_text_color(ui_objects.lbl_temp_value, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_objects.lbl_temp_value, &ui_font_sf_sb_60_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_objects.lbl_temp_value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_objects.lbl_temp_value, "00°");

    // Icon Img
    ui_objects.img_temp_icon = lv_img_create(obj);
    img_set(ui_objects.img_temp_icon, &IMG_INFO_TEMP_NORMAL);

    create_page_indicators(obj, 1);

    tick_screen_temp();
}

void tick_screen_temp() {}

void create_screen_hum(void)
{
    lv_obj_t* obj = lv_obj_create(0);
    ui_objects.screen_hum = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 135, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    create_status_bar(obj, &ui_objects.img_hum_battery, &ui_objects.lbl_hum_batt_pct);

    // Title Label Hum
    ui_objects.lbl_hum_title = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_hum_title, 4, 34);
    lv_obj_set_size(ui_objects.lbl_hum_title, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(ui_objects.lbl_hum_title, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_objects.lbl_hum_title, &ui_font_sf_sb_30_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_objects.lbl_hum_title, "Hum");

    // Status Img
    ui_objects.img_hum_status = lv_img_create(obj);
    img_set(ui_objects.img_hum_status, &IMG_INFO_HUM_DRY);

    // Value Label
    ui_objects.lbl_hum_value = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_hum_value, 0, 72);
    lv_obj_set_size(ui_objects.lbl_hum_value, 135, 49);
    lv_obj_set_style_text_color(ui_objects.lbl_hum_value, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_objects.lbl_hum_value, &ui_font_sf_sb_50_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_objects.lbl_hum_value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_objects.lbl_hum_value, "00%");

    // Icon Img
    ui_objects.img_hum_icon = lv_img_create(obj);
    img_set(ui_objects.img_hum_icon, &IMG_INFO_SAD);

    create_page_indicators(obj, 2);

    tick_screen_hum();
}

void tick_screen_hum() {}

void create_screen_start(void)
{
    lv_obj_t* obj = lv_obj_create(0);
    ui_objects.screen_start = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 135, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_objects.img_start_icon = lv_img_create(obj);
    img_set(ui_objects.img_start_icon, &IMG_INFO_BASE_CENTER);

    ui_objects.spinner_start = lv_spinner_create(obj, 1000, 60);
    lv_obj_set_size(ui_objects.spinner_start, 40, 40);
    lv_obj_align(ui_objects.spinner_start, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_arc_color(ui_objects.spinner_start, lv_color_hex(0xffffffff), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(ui_objects.spinner_start, lv_color_hex(0xff333333), LV_PART_MAIN);
    lv_obj_set_style_arc_width(ui_objects.spinner_start, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ui_objects.spinner_start, 4, LV_PART_MAIN);
}

void create_screen_no_charging(void)
{
    lv_obj_t* obj = lv_obj_create(0);
    ui_objects.screen_no_charging = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 135, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    create_status_bar(obj, &ui_objects.img_no_charging_battery, &ui_objects.lbl_no_charging_batt_pct);

    ui_objects.img_no_charging_icon = lv_img_create(obj);
    img_set(ui_objects.img_no_charging_icon, &IMG_INFO_NO_CHARGING);
}

void create_screen_charging(void)
{
    lv_obj_t* obj = lv_obj_create(0);
    ui_objects.screen_charging = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 135, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    create_status_bar(obj, &ui_objects.img_charging_battery, &ui_objects.lbl_charging_batt_pct);

    ui_objects.img_charging_icon = lv_img_create(obj);
    img_set(ui_objects.img_charging_icon, &IMG_INFO_CHARGING);
}

void create_screen_calibration(void)
{
    lv_obj_t* obj = lv_obj_create(0);
    ui_objects.screen_calibration = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 135, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Status bar
    create_status_bar(obj, &ui_objects.img_calibration_battery, &ui_objects.lbl_calibration_batt_pct);

    // Icon
    ui_objects.img_calibration_icon = lv_img_create(obj);
    img_set(ui_objects.img_calibration_icon, &IMG_INFO_CAT_HUH_CALIB);

    // Text
    ui_objects.lbl_calibration_text = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_calibration_text, 0, 160);
    lv_obj_set_width(ui_objects.lbl_calibration_text, 135);
    lv_obj_set_style_text_color(ui_objects.lbl_calibration_text, lv_color_hex(0xffffffff), 0);
    lv_obj_set_style_text_font(ui_objects.lbl_calibration_text, &ui_font_sf_b_10_digits, 0);
    lv_obj_set_style_text_align(ui_objects.lbl_calibration_text, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(ui_objects.lbl_calibration_text, "Calibrating...\nStab:wait Run:wait");

    // Спиннер
    ui_objects.spinner_calibration = lv_spinner_create(obj, 1000, 60);
    lv_obj_set_size(ui_objects.spinner_calibration, 32, 32);
    lv_obj_align(ui_objects.spinner_calibration, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_set_style_arc_color(ui_objects.spinner_calibration, lv_color_hex(0xffffffff), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(ui_objects.spinner_calibration, lv_color_hex(0xff333333), LV_PART_MAIN);
    lv_obj_set_style_arc_width(ui_objects.spinner_calibration, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ui_objects.spinner_calibration, 4, LV_PART_MAIN);
}

void create_screen_brightness(uint8_t value_percent)
{
    if (value_percent < 5) {
        value_percent = 5;
    } else if (value_percent > 100) {
        value_percent = 100;
    }

    lv_obj_t* obj = lv_obj_create(0);
    ui_objects.screen_brightness = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 135, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    create_status_bar(obj, &ui_objects.img_brightness_battery, &ui_objects.lbl_brightness_batt_pct);

    ui_objects.lbl_brightness_title = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_brightness_title, 0, 38);
    lv_obj_set_width(ui_objects.lbl_brightness_title, 135);
    lv_obj_set_style_text_color(ui_objects.lbl_brightness_title, lv_color_hex(0xffffffff), 0);
    lv_obj_set_style_text_font(ui_objects.lbl_brightness_title, &ui_font_sf_b_10_digits, 0);
    lv_obj_set_style_text_align(ui_objects.lbl_brightness_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(ui_objects.lbl_brightness_title, "Brightness");

    ui_objects.lbl_brightness_value = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_brightness_value, 0, 72);
    lv_obj_set_width(ui_objects.lbl_brightness_value, 135);
    lv_obj_set_style_text_color(ui_objects.lbl_brightness_value, lv_color_hex(0xffffffff), 0);
    lv_obj_set_style_text_font(ui_objects.lbl_brightness_value, &ui_font_sf_sb_60_digits, 0);
    lv_obj_set_style_text_align(ui_objects.lbl_brightness_value, LV_TEXT_ALIGN_CENTER, 0);

    char value_buf[8];
    snprintf(value_buf, sizeof(value_buf), "%u%%", (unsigned int)value_percent);
    lv_label_set_text(ui_objects.lbl_brightness_value, value_buf);

    ui_objects.bar_brightness = lv_bar_create(obj);
    lv_obj_set_pos(ui_objects.bar_brightness, 20, 148);
    lv_obj_set_size(ui_objects.bar_brightness, 95, 10);
    lv_bar_set_range(ui_objects.bar_brightness, 5, 100);
    lv_bar_set_value(ui_objects.bar_brightness, value_percent, LV_ANIM_OFF);
    lv_obj_set_style_radius(ui_objects.bar_brightness, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(ui_objects.bar_brightness, 3, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(ui_objects.bar_brightness, lv_color_hex(0xff202020), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_objects.bar_brightness, lv_color_hex(0xffff6600), LV_PART_INDICATOR);

    ui_objects.lbl_brightness_hint = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_brightness_hint, 0, 178);
    lv_obj_set_width(ui_objects.lbl_brightness_hint, 135);
    lv_obj_set_style_text_color(ui_objects.lbl_brightness_hint, lv_color_hex(0xffa0a0a0), 0);
    lv_obj_set_style_text_font(ui_objects.lbl_brightness_hint, &ui_font_sf_b_10_digits, 0);
    lv_obj_set_style_text_align(ui_objects.lbl_brightness_hint, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(ui_objects.lbl_brightness_hint, "PREV - / NEXT +\nLong NEXT = Done");
}

void create_screen_question(const char* text)
{
    lv_obj_t* obj = lv_obj_create(0);
    ui_objects.screen_question = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 135, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    // Status bar
    create_status_bar(obj, &ui_objects.img_question_battery, &ui_objects.lbl_question_batt_pct);

    ui_objects.img_question_icon = lv_img_create(obj);
    img_set(ui_objects.img_question_icon, &IMG_INFO_CAT_HUH_CENTER);

    ui_objects.lbl_question_text = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_question_text, 0, 155);
    lv_obj_set_width(ui_objects.lbl_question_text, 135);
    lv_obj_set_style_text_color(ui_objects.lbl_question_text, lv_color_hex(0xffffffff), 0);
    lv_obj_set_style_text_font(ui_objects.lbl_question_text, &ui_font_sf_b_10_digits, 0);
    lv_obj_set_style_text_align(ui_objects.lbl_question_text, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(ui_objects.lbl_question_text, text ? text : "Question?");

    // Button Yes
    ui_objects.btn_question_yes = lv_btn_create(obj);
    lv_obj_set_pos(ui_objects.btn_question_yes, 15, 200);
    lv_obj_set_size(ui_objects.btn_question_yes, 50, 28);
    lv_obj_set_style_bg_color(ui_objects.btn_question_yes, lv_color_hex(0xff333333), LV_PART_MAIN);
    lv_obj_set_style_radius(ui_objects.btn_question_yes, 4, LV_PART_MAIN);
    lv_obj_clear_flag(ui_objects.btn_question_yes, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t* lbl_yes = lv_label_create(ui_objects.btn_question_yes);
    lv_label_set_text(lbl_yes, "Yes");
    lv_obj_set_style_text_color(lbl_yes, lv_color_hex(0xffffffff), 0);
    lv_obj_center(lbl_yes);

    // Button NO
    ui_objects.btn_question_no = lv_btn_create(obj);
    lv_obj_set_pos(ui_objects.btn_question_no, 70, 200);
    lv_obj_set_size(ui_objects.btn_question_no, 50, 28);
    lv_obj_set_style_bg_color(ui_objects.btn_question_no, lv_color_hex(0xff333333), LV_PART_MAIN);
    lv_obj_set_style_radius(ui_objects.btn_question_no, 4, LV_PART_MAIN);
    lv_obj_clear_flag(ui_objects.btn_question_no, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t* lbl_no = lv_label_create(ui_objects.btn_question_no);
    lv_label_set_text(lbl_no, "No");
    lv_obj_set_style_text_color(lbl_no, lv_color_hex(0xffffffff), 0);
    lv_obj_center(lbl_no);
}

typedef void (*tick_screen_func_t)();

tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_iaq,
    tick_screen_temp,
    tick_screen_hum,
};

void tick_screen(int screen_index)
{
    tick_screen_funcs[screen_index]();
}

void tick_screen_by_id(enum ScreensEnum screenId)
{
    tick_screen_funcs[screenId - 1]();
}
