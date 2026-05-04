#pragma once

#include <stdbool.h>
#include <lvgl.h>

#define IMG_FACE(name)   "S:/spiffs/img/face/" name ".bin"
#define IMG_STATUS(name) "S:/spiffs/img/status/" name ".bin"
#define IMG_BATT(name)   "S:/spiffs/img/batt/" name ".bin"
#define IMG_MISC(name)   "S:/spiffs/img/misc/" name ".bin"

#define IMG_ULTRA_HAPPY     IMG_FACE("ultra_happy")
#define IMG_HAPPY           IMG_FACE("happy")
#define IMG_ORDINARY        IMG_FACE("ordinary")
#define IMG_SAD             IMG_FACE("sad")
#define IMG_DIZZY           IMG_FACE("dizzy")
#define IMG_DEAD            IMG_FACE("dead")
#define IMG_DIVER           IMG_FACE("diver")
#define IMG_CAT_HUH         IMG_FACE("cat_huh")
#define IMG_ORDINARY_NIMBUS IMG_FACE("ordinary_nimbus")
#define IMG_TEMP_MINUS      IMG_FACE("temp_minus")
#define IMG_TEMP_NORMAL     IMG_FACE("temp_normal")
#define IMG_TEMP_PLUS       IMG_FACE("temp_plus")

#define IMG_GOOD    IMG_STATUS("good")
#define IMG_MID     IMG_STATUS("mid")
#define IMG_BAD     IMG_STATUS("bad")
#define IMG_WARN    IMG_STATUS("warn")
#define IMG_CRIT    IMG_STATUS("crit")
#define IMG_DAMP    IMG_STATUS("damp")
#define IMG_DRY     IMG_STATUS("dry")
#define IMG_MINUS   IMG_STATUS("minus")
#define IMG_NOTHING IMG_STATUS("nothing")
#define IMG_PLUS    IMG_STATUS("plus")

#define IMG_BATT_FULL_NOT_CHARGING IMG_BATT("full_not_charging")
#define IMG_BATT_3_NOT_CHARGING    IMG_BATT("3_not_charging")
#define IMG_BATT_2_NOT_CHARGING    IMG_BATT("2_not_charging")
#define IMG_BATT_1_NOT_CHARGING    IMG_BATT("1_not_charging")
#define IMG_BATT_FULL_CHARGING     IMG_BATT("full_charging")
#define IMG_BATT_3_CHARGING        IMG_BATT("3_charging")
#define IMG_BATT_2_CHARGING        IMG_BATT("2_charging")
#define IMG_BATT_1_CHARGING        IMG_BATT("1_charging")

#define IMG_BASE     IMG_MISC("base")
#define IMG_CHARGING IMG_MISC("lightning_charge")
#define IMG_SUN      IMG_MISC("sun")

typedef struct {
    const char* path;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
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

extern const img_info_t IMG_INFO_BASE_CENTER;
extern const img_info_t IMG_INFO_CHARGING;
extern const img_info_t IMG_INFO_NO_CHARGING;
extern const img_info_t IMG_INFO_CAT_HUH_CENTER;

extern const img_info_t IMG_INFO_SUN;

void img_set(lv_obj_t* img_obj, const img_info_t* info);
void img_set_info(lv_obj_t* img_obj, const img_info_t* info);

const img_info_t* get_battery_info(int percent, bool charging);

const img_info_t* get_iaq_info(int iaq);
const img_info_t* get_iaq_status_info(int iaq);

const img_info_t* get_temp_info(int temp);
const img_info_t* get_temp_status_info(int temp);

const img_info_t* get_hum_info(int hum);
const img_info_t* get_hum_status_info(int hum);
