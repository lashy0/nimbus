#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_LIST_X \
    X(SCREEN_ID_IAQ)  \
    X(SCREEN_ID_TEMP) \
    X(SCREEN_ID_HUM)

enum ScreensEnum {
    SCREEN_ID_NONE = 0,

#define X(name) name,
    SCREEN_LIST_X
#undef X

    SCREEN_COUNT_TOTAL
};

typedef struct {
    lv_obj_t* screen_iaq;
    lv_obj_t* lbl_iaq_value;
    lv_obj_t* lbl_iaq_title;
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
} ui_objects_t;

extern ui_objects_t ui_objects;

void create_screen_iaq();
void tick_screen_iaq();

void create_screen_temp();
void tick_screen_temp();

void create_screen_hum();
void tick_screen_hum();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

#ifdef __cplusplus
}
#endif