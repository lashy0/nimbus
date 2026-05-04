#include "images.h"

#define IMG_FACE(name)   "S:/spiffs/img/face/" name ".bin"
#define IMG_STATUS(name) "S:/spiffs/img/status/" name ".bin"
#define IMG_BATT(name)   "S:/spiffs/img/batt/" name ".bin"
#define IMG_MISC(name)   "S:/spiffs/img/misc/" name ".bin"

static const img_info_t images[IMG_ID_COUNT] = {
    [IMG_ID_BASE_CENTER]      = {IMG_MISC("base"),            32,  83, 71, 73},
    [IMG_ID_ULTRA_HAPPY]      = {IMG_FACE("ultra_happy"),     32, 137, 71, 73},
    [IMG_ID_HAPPY]            = {IMG_FACE("happy"),           32, 137, 71, 73},
    [IMG_ID_ORDINARY]         = {IMG_FACE("ordinary"),        32, 137, 71, 73},
    [IMG_ID_ORDINARY_NIMBUS]  = {IMG_FACE("ordinary_nimbus"), 25,  83, 80, 76},
    [IMG_ID_SAD]              = {IMG_FACE("sad"),             32, 137, 71, 73},
    [IMG_ID_DIZZY]            = {IMG_FACE("dizzy"),           32, 137, 71, 73},
    [IMG_ID_DEAD]             = {IMG_FACE("dead"),            32, 137, 71, 73},
    [IMG_ID_DIVER]            = {IMG_FACE("diver"),           32, 137, 81, 81},
    [IMG_ID_CAT_HUH]          = {IMG_FACE("cat_huh"),         32, 137, 82, 87},
    [IMG_ID_CAT_HUH_CENTER]   = {IMG_FACE("cat_huh"),         32,  45, 82, 87},
    [IMG_ID_NO_CHARGING]      = {IMG_FACE("dead"),            32,  80, 71, 73},
    [IMG_ID_TEMP_MINUS]       = {IMG_FACE("temp_minus"),      31, 127, 82, 98},
    [IMG_ID_TEMP_NORMAL]      = {IMG_FACE("temp_normal"),     31, 127, 82, 83},
    [IMG_ID_TEMP_PLUS]        = {IMG_FACE("temp_plus"),       31, 127, 84, 83},

    [IMG_ID_GOOD]     = {IMG_STATUS("good"),    67, 34, 56, 28},
    [IMG_ID_MID]      = {IMG_STATUS("mid"),     67, 34, 56, 28},
    [IMG_ID_BAD]      = {IMG_STATUS("bad"),     67, 34, 56, 28},
    [IMG_ID_WARN]     = {IMG_STATUS("warn"),    67, 34, 56, 28},
    [IMG_ID_CRIT]     = {IMG_STATUS("crit"),    67, 34, 56, 28},
    [IMG_ID_HUM_GOOD] = {IMG_STATUS("good"),    73, 34, 56, 28},
    [IMG_ID_HUM_DAMP] = {IMG_STATUS("damp"),    73, 34, 56, 28},
    [IMG_ID_HUM_DRY]  = {IMG_STATUS("dry"),     73, 34, 56, 28},
    [IMG_ID_NOTHING]  = {IMG_STATUS("nothing"), 99, 41, 18, 18},
    [IMG_ID_MINUS]    = {IMG_STATUS("minus"),   97, 41, 18, 16},
    [IMG_ID_PLUS]     = {IMG_STATUS("plus"),    97, 41, 18, 16},

    [IMG_ID_BATT_FULL_CHARGING]     = {IMG_BATT("full_charging"),     74,  8, 21, 15},
    [IMG_ID_BATT_3_CHARGING]        = {IMG_BATT("3_charging"),        74,  8, 21, 15},
    [IMG_ID_BATT_2_CHARGING]        = {IMG_BATT("2_charging"),        74,  8, 21, 15},
    [IMG_ID_BATT_1_CHARGING]        = {IMG_BATT("1_charging"),        74,  8, 21, 15},
    [IMG_ID_BATT_FULL_NOT_CHARGING] = {IMG_BATT("full_not_charging"), 74, 11, 21, 10},
    [IMG_ID_BATT_3_NOT_CHARGING]    = {IMG_BATT("3_not_charging"),    74, 11, 21, 10},
    [IMG_ID_BATT_2_NOT_CHARGING]    = {IMG_BATT("2_not_charging"),    74, 11, 21, 10},
    [IMG_ID_BATT_1_NOT_CHARGING]    = {IMG_BATT("1_not_charging"),    74, 11, 21, 10},

    [IMG_ID_CHARGING] = {IMG_MISC("lightning_charge"), 37, 70, 60, 99},
    [IMG_ID_SUN]      = {IMG_MISC("sun"),              40, 40, 55, 55},
};

const img_info_t* img_get(enum img_id id)
{
    return &images[id];
}

void img_set(lv_obj_t* img_obj, const img_info_t* info)
{
    if (!img_obj || !info)
        return;

    lv_obj_set_pos(img_obj, info->x, info->y);
    lv_obj_set_size(img_obj, info->w, info->h);
    lv_img_set_src(img_obj, info->path);
}

void img_set_info(lv_obj_t* img_obj, const img_info_t* info)
{
    if (!img_obj || !info)
        return;

    lv_obj_invalidate(img_obj);

    lv_obj_set_pos(img_obj, info->x, info->y);
    lv_obj_set_size(img_obj, info->w, info->h);
    lv_img_set_src(img_obj, info->path);
}

const img_info_t* get_battery_info(int percent, bool charging)
{
    if (charging) {
        if (percent >= 90) return img_get(IMG_ID_BATT_FULL_CHARGING);
        if (percent >= 60) return img_get(IMG_ID_BATT_3_CHARGING);
        if (percent >= 30) return img_get(IMG_ID_BATT_2_CHARGING);
        return img_get(IMG_ID_BATT_1_CHARGING);
    } else {
        if (percent >= 90) return img_get(IMG_ID_BATT_FULL_NOT_CHARGING);
        if (percent >= 60) return img_get(IMG_ID_BATT_3_NOT_CHARGING);
        if (percent >= 30) return img_get(IMG_ID_BATT_2_NOT_CHARGING);
        return img_get(IMG_ID_BATT_1_NOT_CHARGING);
    }
}

const img_info_t* get_iaq_info(int iaq)
{
    if (iaq <= 50)  return img_get(IMG_ID_ULTRA_HAPPY);
    if (iaq <= 100) return img_get(IMG_ID_HAPPY);
    if (iaq <= 200) return img_get(IMG_ID_ORDINARY);
    if (iaq <= 300) return img_get(IMG_ID_SAD);
    if (iaq <= 400) return img_get(IMG_ID_DIZZY);
    return img_get(IMG_ID_DEAD);
}

const img_info_t* get_iaq_status_info(int iaq)
{
    if (iaq <= 100) return img_get(IMG_ID_GOOD);
    if (iaq <= 200) return img_get(IMG_ID_MID);
    if (iaq <= 300) return img_get(IMG_ID_BAD);
    if (iaq <= 400) return img_get(IMG_ID_WARN);
    return img_get(IMG_ID_CRIT);
}

const img_info_t* get_temp_info(int temp)
{
    if (temp < 0)   return img_get(IMG_ID_TEMP_MINUS);
    if (temp <= 25) return img_get(IMG_ID_TEMP_NORMAL);
    return img_get(IMG_ID_TEMP_PLUS);
}

const img_info_t* get_temp_status_info(int temp)
{
    if (temp < 0)   return img_get(IMG_ID_MINUS);
    if (temp <= 25) return img_get(IMG_ID_NOTHING);
    return img_get(IMG_ID_PLUS);
}

const img_info_t* get_hum_info(int hum)
{
    if (hum < 30) return img_get(IMG_ID_ORDINARY);
    if (hum < 70) return img_get(IMG_ID_HAPPY);
    return img_get(IMG_ID_DIVER);
}

const img_info_t* get_hum_status_info(int hum)
{
    if (hum < 30) return img_get(IMG_ID_HUM_DRY);
    if (hum < 70) return img_get(IMG_ID_HUM_GOOD);
    return img_get(IMG_ID_HUM_DAMP);
}
