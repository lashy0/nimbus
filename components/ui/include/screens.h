#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *iaq;
    lv_obj_t *temp;
    lv_obj_t *hum;
    lv_obj_t *hum100;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *ordinary;
    lv_obj_t *battery_full_not_charging;
    lv_obj_t *good;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *obj7;
    lv_obj_t *obj8;
    lv_obj_t *temp_normal;
    lv_obj_t *battery_full_not_charging_1;
    lv_obj_t *nothing;
    lv_obj_t *obj9;
    lv_obj_t *obj10;
    lv_obj_t *obj11;
    lv_obj_t *obj12;
    lv_obj_t *obj13;
    lv_obj_t *obj14;
    lv_obj_t *ordinary_2;
    lv_obj_t *battery_full_not_charging_2;
    lv_obj_t *good_2;
    lv_obj_t *obj15;
    lv_obj_t *obj16;
    lv_obj_t *obj17;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *obj20;
    lv_obj_t *ordinary_3;
    lv_obj_t *battery_full_not_charging_3;
    lv_obj_t *good_3;
    lv_obj_t *obj21;
    lv_obj_t *obj22;
    lv_obj_t *obj23;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_IAQ = 1,
    SCREEN_ID_TEMP = 2,
    SCREEN_ID_HUM = 3,
    SCREEN_ID_HUM100 = 4,
};

void create_screen_iaq();
void tick_screen_iaq();

void create_screen_temp();
void tick_screen_temp();

void create_screen_hum();
void tick_screen_hum();

void create_screen_hum100();
void tick_screen_hum100();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/