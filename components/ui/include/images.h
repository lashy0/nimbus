#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_ordinary;
extern const lv_img_dsc_t img_full_not_charging;
extern const lv_img_dsc_t img_good;
extern const lv_img_dsc_t img_dead;
extern const lv_img_dsc_t img_nothing;
extern const lv_img_dsc_t img_temp_normal;
extern const lv_img_dsc_t img_happy;
extern const lv_img_dsc_t img_base;
extern const lv_img_dsc_t img_battery_3_not_charging;
extern const lv_img_dsc_t img_battery_2_not_charging;
extern const lv_img_dsc_t img_battery_1_not_charging;
extern const lv_img_dsc_t img_battery_1_charging;
extern const lv_img_dsc_t img_battery_2_charging;
extern const lv_img_dsc_t img_battery_3_charging;
extern const lv_img_dsc_t img_battery_full_charging;
extern const lv_img_dsc_t img_crit;
extern const lv_img_dsc_t img_bad;
extern const lv_img_dsc_t img_damp;
extern const lv_img_dsc_t img_mid;
extern const lv_img_dsc_t img_dry;
extern const lv_img_dsc_t img_minus;
extern const lv_img_dsc_t img_plus;
extern const lv_img_dsc_t img_diver;
extern const lv_img_dsc_t img_dizzy;
extern const lv_img_dsc_t img_sad;
extern const lv_img_dsc_t img_temp_minus;
extern const lv_img_dsc_t img_temp_plus;
extern const lv_img_dsc_t img_ordinary_nimbus;
extern const lv_img_dsc_t img_cat_huh;
extern const lv_img_dsc_t img_ultra_happy;
extern const lv_img_dsc_t img_choice_no;
extern const lv_img_dsc_t img_choice_yes;
extern const lv_img_dsc_t img_lightning_charge;
extern const lv_img_dsc_t img_warn;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[34];


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/