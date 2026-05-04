#pragma once

#include <stdbool.h>
#include <lvgl.h>

typedef struct {
    const char* path;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} img_info_t;

enum img_id {
    IMG_ID_BASE,
    IMG_ID_ULTRA_HAPPY,
    IMG_ID_HAPPY,
    IMG_ID_ORDINARY,
    IMG_ID_ORDINARY_NIMBUS,
    IMG_ID_SAD,
    IMG_ID_DIZZY,
    IMG_ID_DEAD,
    IMG_ID_DIVER,
    IMG_ID_CAT_HUH,
    IMG_ID_TEMP_MINUS,
    IMG_ID_TEMP_NORMAL,
    IMG_ID_TEMP_PLUS,

    IMG_ID_GOOD,
    IMG_ID_MID,
    IMG_ID_BAD,
    IMG_ID_WARN,
    IMG_ID_CRIT,
    IMG_ID_HUM_GOOD,
    IMG_ID_HUM_DAMP,
    IMG_ID_HUM_DRY,
    IMG_ID_NOTHING,
    IMG_ID_MINUS,
    IMG_ID_PLUS,

    IMG_ID_BATT_FULL_CHARGING,
    IMG_ID_BATT_3_CHARGING,
    IMG_ID_BATT_2_CHARGING,
    IMG_ID_BATT_1_CHARGING,
    IMG_ID_BATT_FULL_NOT_CHARGING,
    IMG_ID_BATT_3_NOT_CHARGING,
    IMG_ID_BATT_2_NOT_CHARGING,
    IMG_ID_BATT_1_NOT_CHARGING,

    IMG_ID_CHARGING,
    IMG_ID_SUN,

    IMG_ID_COUNT,
};

const img_info_t* img_get(enum img_id id);

void img_set(lv_obj_t* img_obj, const img_info_t* info);
void img_set_info(lv_obj_t* img_obj, const img_info_t* info);
void img_set_at(lv_obj_t* img_obj, enum img_id id, int16_t x, int16_t y);

const img_info_t* get_battery_info(int percent, bool charging);

const img_info_t* get_iaq_info(int iaq);
const img_info_t* get_iaq_status_info(int iaq);

const img_info_t* get_temp_info(int temp);
const img_info_t* get_temp_status_info(int temp);

const img_info_t* get_hum_info(int hum);
const img_info_t* get_hum_status_info(int hum);
