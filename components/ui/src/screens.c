#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;
uint32_t active_theme_index = 0;

void create_screen_iaq() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.iaq = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 135, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 50, 225);
            lv_obj_set_size(obj, 5, 5);
            lv_obj_set_style_arc_rounded(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj1 = obj;
            lv_obj_set_pos(obj, 65, 225);
            lv_obj_set_size(obj, 5, 5);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj2 = obj;
            lv_obj_set_pos(obj, 80, 225);
            lv_obj_set_size(obj, 5, 5);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // ordinary
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.ordinary = obj;
            lv_obj_set_pos(obj, 32, 137);
            lv_obj_set_size(obj, 71, 73);
            lv_obj_set_style_arc_img_src(obj, &img_ordinary, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_img_src(obj, &img_happy, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // battery-full-not-charging
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.battery_full_not_charging = obj;
            lv_obj_set_pos(obj, 74, 11);
            lv_obj_set_size(obj, 21, 10);
            lv_obj_set_style_bg_img_src(obj, &img_full_not_charging, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_img_src(obj, &img_full_not_charging, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // good
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.good = obj;
            lv_obj_set_pos(obj, 67, 34);
            lv_obj_set_size(obj, 56, 28);
            lv_obj_set_style_arc_img_src(obj, &img_good, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_img_src(obj, &img_good, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj3 = obj;
            lv_obj_set_pos(obj, 0, 72);
            lv_obj_set_size(obj, 135, 49);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sf_sb_60_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "069");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj4 = obj;
            lv_obj_set_pos(obj, 13, 34);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sf_sb_30_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "IAQ");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj5 = obj;
            lv_obj_set_pos(obj, 41, 12);
            lv_obj_set_size(obj, 30, 8);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sf_b_10_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "100 %");
        }
    }
    
    tick_screen_iaq();
}

void tick_screen_iaq() {
}

void create_screen_temp() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.temp = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 135, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj6 = obj;
            lv_obj_set_pos(obj, 50, 225);
            lv_obj_set_size(obj, 5, 5);
            lv_obj_set_style_arc_rounded(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj7 = obj;
            lv_obj_set_pos(obj, 65, 225);
            lv_obj_set_size(obj, 5, 5);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj8 = obj;
            lv_obj_set_pos(obj, 80, 225);
            lv_obj_set_size(obj, 5, 5);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // temp_normal
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.temp_normal = obj;
            lv_obj_set_pos(obj, 31, 127);
            lv_obj_set_size(obj, 82, 83);
            lv_obj_set_style_arc_img_src(obj, &img_ordinary, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_img_src(obj, &img_temp_normal, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // battery_full_not_charging_1
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.battery_full_not_charging_1 = obj;
            lv_obj_set_pos(obj, 74, 11);
            lv_obj_set_size(obj, 21, 10);
            lv_obj_set_style_bg_img_src(obj, &img_full_not_charging, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_img_src(obj, &img_full_not_charging, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // nothing
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.nothing = obj;
            lv_obj_set_pos(obj, 99, 41);
            lv_obj_set_size(obj, 18, 18);
            lv_obj_set_style_arc_img_src(obj, &img_good, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_img_src(obj, &img_nothing, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj9 = obj;
            lv_obj_set_pos(obj, 0, 72);
            lv_obj_set_size(obj, 135, 49);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sf_sb_60_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "15°");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj10 = obj;
            lv_obj_set_pos(obj, 20, 34);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sf_sb_30_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Temp");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj11 = obj;
            lv_obj_set_pos(obj, 41, 12);
            lv_obj_set_size(obj, 30, 8);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sf_b_10_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "100 %");
        }
    }
    
    tick_screen_temp();
}

void tick_screen_temp() {
}

void create_screen_hum() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.hum = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 135, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj12 = obj;
            lv_obj_set_pos(obj, 50, 225);
            lv_obj_set_size(obj, 5, 5);
            lv_obj_set_style_arc_rounded(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj13 = obj;
            lv_obj_set_pos(obj, 65, 225);
            lv_obj_set_size(obj, 5, 5);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj14 = obj;
            lv_obj_set_pos(obj, 80, 225);
            lv_obj_set_size(obj, 5, 5);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // ordinary_2
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.ordinary_2 = obj;
            lv_obj_set_pos(obj, 32, 137);
            lv_obj_set_size(obj, 71, 73);
            lv_obj_set_style_arc_img_src(obj, &img_ordinary, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_img_src(obj, &img_happy, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // battery_full_not_charging_2
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.battery_full_not_charging_2 = obj;
            lv_obj_set_pos(obj, 74, 11);
            lv_obj_set_size(obj, 21, 10);
            lv_obj_set_style_bg_img_src(obj, &img_full_not_charging, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_img_src(obj, &img_full_not_charging, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // good_2
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.good_2 = obj;
            lv_obj_set_pos(obj, 73, 34);
            lv_obj_set_size(obj, 56, 28);
            lv_obj_set_style_arc_img_src(obj, &img_good, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_img_src(obj, &img_good, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj15 = obj;
            lv_obj_set_pos(obj, 0, 72);
            lv_obj_set_size(obj, 135, 49);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sf_sb_60_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "45%");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj16 = obj;
            lv_obj_set_pos(obj, 4, 34);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sf_sb_30_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Hum");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj17 = obj;
            lv_obj_set_pos(obj, 41, 12);
            lv_obj_set_size(obj, 30, 8);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sf_b_10_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "100 %");
        }
    }
    
    tick_screen_hum();
}

void tick_screen_hum() {
}

void create_screen_hum100() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.hum100 = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 135, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj18 = obj;
            lv_obj_set_pos(obj, 50, 225);
            lv_obj_set_size(obj, 5, 5);
            lv_obj_set_style_arc_rounded(obj, true, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj19 = obj;
            lv_obj_set_pos(obj, 65, 225);
            lv_obj_set_size(obj, 5, 5);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff808080), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj20 = obj;
            lv_obj_set_pos(obj, 80, 225);
            lv_obj_set_size(obj, 5, 5);
            lv_obj_set_style_border_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // ordinary_3
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.ordinary_3 = obj;
            lv_obj_set_pos(obj, 32, 137);
            lv_obj_set_size(obj, 71, 73);
            lv_obj_set_style_arc_img_src(obj, &img_ordinary, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_img_src(obj, &img_happy, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // battery_full_not_charging_3
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.battery_full_not_charging_3 = obj;
            lv_obj_set_pos(obj, 74, 11);
            lv_obj_set_size(obj, 21, 10);
            lv_obj_set_style_bg_img_src(obj, &img_full_not_charging, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_img_src(obj, &img_full_not_charging, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // good_3
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.good_3 = obj;
            lv_obj_set_pos(obj, 73, 34);
            lv_obj_set_size(obj, 56, 28);
            lv_obj_set_style_arc_img_src(obj, &img_good, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_img_src(obj, &img_good, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj21 = obj;
            lv_obj_set_pos(obj, 0, 72);
            lv_obj_set_size(obj, 135, 49);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sf_sb_50_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "100%");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj22 = obj;
            lv_obj_set_pos(obj, 4, 34);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sf_sb_30_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "Hum");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj23 = obj;
            lv_obj_set_pos(obj, 41, 12);
            lv_obj_set_size(obj, 30, 8);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sf_b_10_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "100 %");
        }
    }
    
    tick_screen_hum100();
}

void tick_screen_hum100() {
}



typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_iaq,
    tick_screen_temp,
    tick_screen_hum,
    tick_screen_hum100,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_iaq();
    create_screen_temp();
    create_screen_hum();
    create_screen_hum100();
}
