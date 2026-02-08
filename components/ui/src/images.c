#include "images.h"

const img_info_t IMG_INFO_ULTRA_HAPPY = {IMG_ULTRA_HAPPY, 32, 137, 71, 73};
const img_info_t IMG_INFO_HAPPY = {IMG_HAPPY, 32, 137, 71, 73};
const img_info_t IMG_INFO_ORDINARY = {IMG_ORDINARY, 32, 137, 71, 73};
const img_info_t IMG_INFO_SAD = {IMG_SAD, 32, 137, 71, 73};
const img_info_t IMG_INFO_DIZZY = {IMG_DIZZY, 32, 137, 71, 73};
const img_info_t IMG_INFO_DEAD = {IMG_DEAD, 32, 137, 71, 73};
const img_info_t IMG_INFO_TEMP_MINUS = {IMG_TEMP_MINUS, 31, 127, 82, 98};
const img_info_t IMG_INFO_TEMP_NORMAL = {IMG_TEMP_NORMAL, 31, 127, 82, 83};
const img_info_t IMG_INFO_TEMP_PLUS = {IMG_TEMP_PLUS, 31, 127, 84, 83};
const img_info_t IMG_INFO_DIVER = {IMG_DIVER, 32, 137, 81, 81};
const img_info_t IMG_INFO_CAT_HUH = {IMG_CAT_HUH, 32, 137, 82, 87};
const img_info_t IMG_INFO_ORDINARY_NIMBUS = {IMG_ORDINARY_NIMBUS, 32, 137, 80, 76};

const img_info_t IMG_INFO_GOOD    = { IMG_GOOD,    67, 34, 56, 28 };
const img_info_t IMG_INFO_MID     = { IMG_MID,     67, 34, 56, 28 };
const img_info_t IMG_INFO_BAD     = { IMG_BAD,     67, 34, 56, 28 };
const img_info_t IMG_INFO_WARN    = { IMG_WARN,    67, 34, 56, 28 };
const img_info_t IMG_INFO_CRIT    = { IMG_CRIT,    67, 34, 56, 28 };

const img_info_t IMG_INFO_NOTHING = { IMG_NOTHING, 99, 41, 18, 18 };
const img_info_t IMG_INFO_MINUS   = { IMG_MINUS,   97, 41, 18, 16 };
const img_info_t IMG_INFO_PLUS    = { IMG_PLUS,    97, 41, 18, 16 };

const img_info_t IMG_INFO_HUM_GOOD = { IMG_GOOD, 73, 34, 56, 28 };
const img_info_t IMG_INFO_HUM_DAMP = { IMG_DAMP, 73, 34, 56, 28 };
const img_info_t IMG_INFO_HUM_DRY  = { IMG_DRY,  73, 34, 56, 28 };

const img_info_t IMG_INFO_BATT_FULL_CHARGING     = { IMG_BATT_FULL_CHARGING,     74, 8, 21, 15 };
const img_info_t IMG_INFO_BATT_3_CHARGING        = { IMG_BATT_3_CHARGING,        74, 8, 21, 15 };
const img_info_t IMG_INFO_BATT_2_CHARGING        = { IMG_BATT_2_CHARGING,        74, 8, 21, 15 };
const img_info_t IMG_INFO_BATT_1_CHARGING        = { IMG_BATT_1_CHARGING,        74, 8, 21, 15 };
const img_info_t IMG_INFO_BATT_FULL_NOT_CHARGING = { IMG_BATT_FULL_NOT_CHARGING, 74, 11, 21, 10 };
const img_info_t IMG_INFO_BATT_3_NOT_CHARGING    = { IMG_BATT_3_NOT_CHARGING,    74, 11, 21, 10 };
const img_info_t IMG_INFO_BATT_2_NOT_CHARGING    = { IMG_BATT_2_NOT_CHARGING,    74, 11, 21, 10 };
const img_info_t IMG_INFO_BATT_1_NOT_CHARGING    = { IMG_BATT_1_NOT_CHARGING,    74, 11, 21, 10 };

void img_set(lv_obj_t* img_obj, const img_info_t* info)
{
    if (!img_obj || !info) return;

    lv_obj_set_pos(img_obj, info->x, info->y);
    lv_obj_set_size(img_obj, info->w, info->h);
    lv_img_set_src(img_obj, info->path);
}

void img_set_info(lv_obj_t* img_obj, const img_info_t* info)
{
    if (!img_obj || !info) return;

    lv_obj_invalidate(img_obj);

    lv_obj_set_pos(img_obj, info->x, info->y);
    lv_obj_set_size(img_obj, info->w, info->h);
    lv_img_set_src(img_obj, info->path);
}

const char* get_battery_icon(int percent, bool charging) {
    return get_battery_info(percent, charging)->path;
}

const img_info_t* get_battery_info(int percent, bool charging) {
    if (charging) {
        if (percent >= 90) return &IMG_INFO_BATT_FULL_CHARGING;
        if (percent >= 60) return &IMG_INFO_BATT_3_CHARGING;
        if (percent >= 30) return &IMG_INFO_BATT_2_CHARGING;
        return &IMG_INFO_BATT_1_CHARGING;
    } else {
        if (percent >= 90) return &IMG_INFO_BATT_FULL_NOT_CHARGING;
        if (percent >= 60) return &IMG_INFO_BATT_3_NOT_CHARGING;
        if (percent >= 30) return &IMG_INFO_BATT_2_NOT_CHARGING;
        return &IMG_INFO_BATT_1_NOT_CHARGING;
    }
}

const char* get_iaq_icon(int iaq) {
    return get_iaq_info(iaq)->path;
}

const img_info_t* get_iaq_info(int iaq) {
    if (iaq <= 50)  return &IMG_INFO_ULTRA_HAPPY;
    if (iaq <= 100)  return &IMG_INFO_HAPPY;
    if (iaq <= 200) return &IMG_INFO_ORDINARY;
    if (iaq <= 300) return &IMG_INFO_SAD;
    if (iaq <= 400) return &IMG_INFO_DIZZY;
    return &IMG_INFO_DEAD;
}

const char* get_iaq_status_icon(int iaq) {
    return get_iaq_status_info(iaq)->path;
}

const img_info_t* get_iaq_status_info(int iaq) {
    if (iaq <= 50)  return &IMG_INFO_GOOD;
    if (iaq <= 100) return &IMG_INFO_GOOD;
    if (iaq <= 200) return &IMG_INFO_MID;
    if (iaq <= 300) return &IMG_INFO_BAD;
    if (iaq <= 400) return &IMG_INFO_WARN;
    return &IMG_INFO_CRIT;
}

const char* get_temp_icon(int temp) {
    return get_temp_info(temp)->path;
}

const img_info_t* get_temp_info(int temp) {
    if (temp < 0)  return &IMG_INFO_TEMP_MINUS;
    if (temp <= 25) return &IMG_INFO_TEMP_NORMAL;
    return &IMG_INFO_TEMP_PLUS;
}

const char* get_temp_status_icon(int temp) {
    return get_temp_status_info(temp)->path;
}

const img_info_t* get_temp_status_info(int temp) {
    if (temp < 0)  return &IMG_INFO_MINUS;
    if (temp <= 25) return &IMG_INFO_NOTHING;
    return &IMG_INFO_PLUS;
}

const char* get_hum_icon(int hum) {
    return get_hum_info(hum)->path;
}

const img_info_t* get_hum_info(int hum) {
    if (hum < 30)  return &IMG_INFO_ORDINARY;
    if (hum < 70) return &IMG_INFO_HAPPY;
    return &IMG_INFO_DIVER;
}

const char* get_hum_status_icon(int hum) {
    return get_hum_status_info(hum)->path;
}

const img_info_t* get_hum_status_info(int hum) {
    if (hum < 30)  return &IMG_INFO_HUM_DRY;
    if (hum < 70) return &IMG_INFO_HUM_GOOD;
    return &IMG_INFO_HUM_DAMP;
}