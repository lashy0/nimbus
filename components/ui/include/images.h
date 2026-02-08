#pragma once

#include <stdbool.h>
#include <lvgl.h>

#define IMG_PATH(name) "S:/spiffs/img_" name ".bin"

#define IMG_ULTRA_HAPPY       IMG_PATH("ultra_happy")
#define IMG_HAPPY             IMG_PATH("happy")
#define IMG_ORDINARY          IMG_PATH("ordinary")
#define IMG_SAD               IMG_PATH("sad")
#define IMG_DIZZY             IMG_PATH("dizzy")
#define IMG_DEAD              IMG_PATH("dead")
#define IMG_TEMP_MINUS        IMG_PATH("temp_minus")
#define IMG_TEMP_NORMAL       IMG_PATH("temp_normal")
#define IMG_TEMP_PLUS         IMG_PATH("temp_plus")
#define IMG_DIVER             IMG_PATH("diver")
#define IMG_CAT_HUH           IMG_PATH("cat_huh")
#define IMG_ORDINARY_NIMBUS   IMG_PATH("ordinary_numbus")
#define IMG_HAPPY_CLOSED_EYES IMG_PATH("happy_closed_eyes")

#define IMG_BAD  IMG_PATH("bad")
#define IMG_CRIT IMG_PATH("crit")
#define IMG_WARN IMG_PATH("warn")
#define IMG_DAMP IMG_PATH("damp")
#define IMG_DRY  IMG_PATH("dry")
#define IMG_MID  IMG_PATH("mid")
#define IMG_GOOD IMG_PATH("good")

#define IMG_MINUS   IMG_PATH("minus")
#define IMG_NOTHING IMG_PATH("nothing")
#define IMG_PLUS    IMG_PATH("plus")

#define IMG_BATT_FULL_NOT_CHARGING IMG_PATH("batt_full_not_charging")
#define IMG_BATT_3_NOT_CHARGING    IMG_PATH("batt_3_not_charging")
#define IMG_BATT_2_NOT_CHARGING    IMG_PATH("batt_2_not_charging")
#define IMG_BATT_1_NOT_CHARGING    IMG_PATH("batt_1_not_charging")
#define IMG_BATT_FULL_CHARGING     IMG_PATH("batt_full_charging")
#define IMG_BATT_3_CHARGING        IMG_PATH("batt_3_charging")
#define IMG_BATT_2_CHARGING        IMG_PATH("batt_2_charging")
#define IMG_BATT_1_CHARGING        IMG_PATH("batt_1_charging")

typedef struct {
    const char* path;
    int16_t     x;
    int16_t     y;
    int16_t     w;
    int16_t     h;
} img_info_t;

extern const img_info_t IMG_INFO_ULTRA_HAPPY;
extern const img_info_t IMG_INFO_HAPPY;
extern const img_info_t IMG_INFO_ORDINARY;
extern const img_info_t IMG_INFO_SAD;
extern const img_info_t IMG_INFO_DIZZY;
extern const img_info_t IMG_INFO_DEAD;
extern const img_info_t IMG_INFO_TEMP_MINUS;
extern const img_info_t IMG_INFO_TEMP_NORMAL;
extern const img_info_t IMG_INFO_TEMP_PLUS;
extern const img_info_t IMG_INFO_DIVER;
extern const img_info_t IMG_INFO_CAT_HUH;
extern const img_info_t IMG_INFO_ORDINARY_NIMBUS;

extern const img_info_t IMG_INFO_GOOD;
extern const img_info_t IMG_INFO_MID;
extern const img_info_t IMG_INFO_BAD;
extern const img_info_t IMG_INFO_WARN;
extern const img_info_t IMG_INFO_CRIT;

extern const img_info_t IMG_INFO_NOTHING;
extern const img_info_t IMG_INFO_MINUS;
extern const img_info_t IMG_INFO_PLUS;

extern const img_info_t IMG_INFO_HUM_GOOD;
extern const img_info_t IMG_INFO_HUM_DAMP;
extern const img_info_t IMG_INFO_HUM_DRY;

extern const img_info_t IMG_INFO_BATT_FULL_CHARGING;
extern const img_info_t IMG_INFO_BATT_3_CHARGING;
extern const img_info_t IMG_INFO_BATT_2_CHARGING;
extern const img_info_t IMG_INFO_BATT_1_CHARGING;
extern const img_info_t IMG_INFO_BATT_FULL_NOT_CHARGING;
extern const img_info_t IMG_INFO_BATT_3_NOT_CHARGING;
extern const img_info_t IMG_INFO_BATT_2_NOT_CHARGING;
extern const img_info_t IMG_INFO_BATT_1_NOT_CHARGING;

void img_set(lv_obj_t* img_obj, const img_info_t* info);
void img_set_info(lv_obj_t* img_obj, const img_info_t* info);

const char* get_battery_icon(int percent, bool charging);
const img_info_t* get_battery_info(int percent, bool charging);

const char* get_iaq_icon(int iaq);
const img_info_t* get_iaq_info(int iaq);
const char* get_iaq_status_icon(int iaq);
const img_info_t* get_iaq_status_info(int iaq);

const char* get_temp_icon(int temp);
const img_info_t* get_temp_info(int temp);
const char* get_temp_status_icon(int temp);
const img_info_t* get_temp_status_info(int temp);

const char* get_hum_icon(int hum);
const img_info_t* get_hum_info(int hum);
const char* get_hum_status_icon(int hum);
const img_info_t* get_hum_status_info(int hum);