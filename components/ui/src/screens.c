#include "screens.h"

#include "fonts.h"
#include "images.h"
#include "ui.h"

ui_objects_t ui_objects;

static void create_status_bar(lv_obj_t* parent, lv_obj_t** out_img_batt, lv_obj_t** out_lbl_pct)
{
    lv_obj_t* img = lv_img_create(parent);
    *out_img_batt = img;
    img_set(img, &IMG_INFO_BATT_FULL_NOT_CHARGING);

    lv_obj_t* lbl = lv_label_create(parent);
    *out_lbl_pct = lbl;
    lv_obj_set_pos(lbl, 41, 12);
    lv_obj_set_size(lbl, 30, 8);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffffff), 0);
    lv_obj_set_style_text_font(lbl, &ui_font_sf_b_10_digits, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_text(lbl, "00 %");
}

static void create_page_indicators(lv_obj_t* parent, int active_index)
{
    int x_positions[] = {50, 65, 80};
    int y_pos = 225;

    for (int i = 0; i < 3; i++)
    {
        lv_obj_t* obj = lv_obj_create(parent);

        lv_obj_set_pos(obj, x_positions[i], y_pos);
        lv_obj_set_size(obj, 5, 5);
        lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

        lv_color_t color = (i == active_index) ? lv_color_hex(0xffffffff) : lv_color_hex(0xff808080);

        lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void create_screen_iaq()
{
    lv_obj_t* obj = lv_obj_create(0);
    ui_objects.screen_iaq = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 135, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    create_status_bar(obj, &ui_objects.img_iaq_battery, &ui_objects.lbl_iaq_batt_pct);

    // Title Label IAQ
    ui_objects.lbl_iaq_title = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_iaq_title, 13, 34);
    lv_obj_set_size(ui_objects.lbl_iaq_title, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(ui_objects.lbl_iaq_title, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_objects.lbl_iaq_title, &ui_font_sf_sb_30_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_objects.lbl_iaq_title, "IAQ");

    // Status Img
    ui_objects.img_iaq_status = lv_img_create(obj);
    img_set(ui_objects.img_iaq_status, &IMG_INFO_GOOD);

    // Value Label
    ui_objects.lbl_iaq_value = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_iaq_value, 0, 72);
    lv_obj_set_size(ui_objects.lbl_iaq_value, 135, 49);
    lv_obj_set_style_text_color(ui_objects.lbl_iaq_value, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_objects.lbl_iaq_value, &ui_font_sf_sb_60_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_objects.lbl_iaq_value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_objects.lbl_iaq_value, "000");

    // Icon Img
    ui_objects.img_iaq_icon = lv_img_create(obj);
    img_set(ui_objects.img_iaq_icon, &IMG_INFO_ULTRA_HAPPY);

    create_page_indicators(obj, 0);

    tick_screen_iaq();
}

void tick_screen_iaq() {}

void create_screen_temp()
{
    lv_obj_t* obj = lv_obj_create(0);
    ui_objects.screen_temp = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 135, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    create_status_bar(obj, &ui_objects.img_temp_battery, &ui_objects.lbl_temp_batt_pct);

    // Title Label Temp
    ui_objects.lbl_temp_title = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_temp_title, 20, 34);
    lv_obj_set_size(ui_objects.lbl_temp_title, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(ui_objects.lbl_temp_title, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_objects.lbl_temp_title, &ui_font_sf_sb_30_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_objects.lbl_temp_title, "Temp");

    // Status Img
    ui_objects.img_temp_status = lv_img_create(obj);
    img_set(ui_objects.img_temp_status, &IMG_INFO_NOTHING);

    // Value Label
    ui_objects.lbl_temp_value = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_temp_value, 0, 72);
    lv_obj_set_size(ui_objects.lbl_temp_value, 135, 49);
    lv_obj_set_style_text_color(ui_objects.lbl_temp_value, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_objects.lbl_temp_value, &ui_font_sf_sb_60_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_objects.lbl_temp_value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_objects.lbl_temp_value, "00°");

    // Icon Img
    ui_objects.img_temp_icon = lv_img_create(obj);
    img_set(ui_objects.img_temp_icon, &IMG_INFO_TEMP_NORMAL);

    create_page_indicators(obj, 1);

    tick_screen_temp();
}

void tick_screen_temp() {}

void create_screen_hum()
{
    lv_obj_t* obj = lv_obj_create(0);
    ui_objects.screen_hum = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 135, 240);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);

    create_status_bar(obj, &ui_objects.img_hum_battery, &ui_objects.lbl_hum_batt_pct);

    // Title Label Hum
    ui_objects.lbl_hum_title = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_hum_title, 4, 34);
    lv_obj_set_size(ui_objects.lbl_hum_title, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_text_color(ui_objects.lbl_hum_title, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_objects.lbl_hum_title, &ui_font_sf_sb_30_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_objects.lbl_hum_title, "Hum");

    // Status Img
    ui_objects.img_hum_status = lv_img_create(obj);
    img_set(ui_objects.img_hum_status, &IMG_INFO_HUM_DRY);

    // Value Label
    ui_objects.lbl_hum_value = lv_label_create(obj);
    lv_obj_set_pos(ui_objects.lbl_hum_value, 0, 72);
    lv_obj_set_size(ui_objects.lbl_hum_value, 135, 49);
    lv_obj_set_style_text_color(ui_objects.lbl_hum_value, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_objects.lbl_hum_value, &ui_font_sf_sb_50_digits, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(ui_objects.lbl_hum_value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(ui_objects.lbl_hum_value, "00%");

    // Icon Img
    ui_objects.img_hum_icon = lv_img_create(obj);
    img_set(ui_objects.img_hum_icon, &IMG_INFO_SAD);

    create_page_indicators(obj, 2);

    tick_screen_hum();
}

void tick_screen_hum() {}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_iaq,
    tick_screen_temp,
    tick_screen_hum,
};
void tick_screen(int screen_index) { tick_screen_funcs[screen_index](); }
void tick_screen_by_id(enum ScreensEnum screenId) { tick_screen_funcs[screenId - 1](); }
