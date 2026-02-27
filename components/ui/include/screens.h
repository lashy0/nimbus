#pragma once

#include <lvgl.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_LIST_X                                                                                                  \
    X(SCREEN_ID_IAQ)                                                                                                   \
    X(SCREEN_ID_TEMP)                                                                                                  \
    X(SCREEN_ID_HUM)

enum ScreensEnum {
    SCREEN_ID_NONE = 0,

#define X(name) name,
    SCREEN_LIST_X
#undef X

        SCREEN_ID_START,
    SCREEN_ID_NO_CHARGING,
    SCREEN_ID_CHARGING,
    SCREEN_ID_CALIBRATION,
    SCREEN_ID_BRIGHTNESS,
    SCREEN_ID_QUESTION,
    SCREEN_COUNT_TOTAL
};

typedef struct {
    lv_obj_t* screen_iaq;
    lv_obj_t* lbl_iaq_value;
    lv_obj_t* lbl_iaq_title;
    lv_obj_t* lbl_iaq_cal_status;
    lv_obj_t* lbl_iaq_batt_pct;
    lv_obj_t* img_iaq_icon;
    lv_obj_t* img_iaq_status;
    lv_obj_t* img_iaq_battery;

    lv_obj_t* screen_temp;
    lv_obj_t* lbl_temp_value;
    lv_obj_t* lbl_temp_title;
    lv_obj_t* lbl_temp_batt_pct;
    lv_obj_t* img_temp_icon;
    lv_obj_t* img_temp_status;
    lv_obj_t* img_temp_battery;

    lv_obj_t* screen_hum;
    lv_obj_t* lbl_hum_value;
    lv_obj_t* lbl_hum_title;
    lv_obj_t* lbl_hum_batt_pct;
    lv_obj_t* img_hum_icon;
    lv_obj_t* img_hum_status;
    lv_obj_t* img_hum_battery;

    lv_obj_t* screen_start;
    lv_obj_t* img_start_icon;
    lv_obj_t* spinner_start;

    lv_obj_t* screen_no_charging;
    lv_obj_t* img_no_charging_icon;

    lv_obj_t* screen_charging;
    lv_obj_t* img_charging_icon;

    lv_obj_t* screen_calibration;
    lv_obj_t* img_calibration_icon;
    lv_obj_t* lbl_calibration_text;
    lv_obj_t* spinner_calibration;
    lv_obj_t* lbl_calibration_batt_pct;
    lv_obj_t* img_calibration_battery;

    lv_obj_t* screen_brightness;
    lv_obj_t* lbl_brightness_title;
    lv_obj_t* lbl_brightness_value;
    lv_obj_t* bar_brightness;
    lv_obj_t* lbl_brightness_hint;
    lv_obj_t* lbl_brightness_batt_pct;
    lv_obj_t* img_brightness_battery;

    lv_obj_t* screen_question;
    lv_obj_t* img_question_icon;
    lv_obj_t* lbl_question_text;
    lv_obj_t* btn_question_yes;
    lv_obj_t* btn_question_no;
    lv_obj_t* lbl_question_batt_pct;
    lv_obj_t* img_question_battery;
} ui_objects_t;

extern ui_objects_t ui_objects;

void create_screen_iaq(void);
void tick_screen_iaq();

void create_screen_temp(void);
void tick_screen_temp();

void create_screen_hum(void);
void tick_screen_hum();

void create_screen_start(void);

void create_screen_no_charging(void);

void create_screen_charging(void);

void create_screen_calibration(void);

void create_screen_brightness(uint8_t value_percent);

void create_screen_question(const char* text);

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

#ifdef __cplusplus
}
#endif
