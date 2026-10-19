// Temporary bring-up view until app supplies a ui_model: checks panel offsets and
// orientation, every runtime image and font, and their heap cost.
#include <stdio.h>

#include "esp_log.h"
#include "ui_internal.h"

#define IMAGE_PERIOD_MS 2000
#define ASSET_PATH_MAX 64

static const char* TAG = "ui_bringup";

static const char* const IMAGES[] = {
    "face/ordinary",
    "face/ordinary_closed_eyes",
    "face/ordinary_nimbus",
    "face/happy",
    "face/happy_closed_eyes",
    "face/ultra_happy",
    "face/sad",
    "face/dizzy",
    "face/dead",
    "face/cat_huh",
    "face/diver",
    "face/diver_closed_eyes",
    "face/temp_minus",
    "face/temp_minus_closed_eyes",
    "face/temp_normal",
    "face/temp_normal_closed_eyes",
    "face/temp_plus",
    "misc/base",
    "misc/lightning_charge",
    "misc/sun",
    "status/good",
    "status/mid",
    "status/bad",
    "status/warn",
    "status/crit",
    "status/damp",
    "status/dry",
    "status/minus",
    "status/nothing",
    "status/plus",
    "batt/1_charging",
    "batt/1_not_charging",
    "batt/2_charging",
    "batt/2_not_charging",
    "batt/3_charging",
    "batt/3_not_charging",
    "batt/full_charging",
    "batt/full_not_charging",
};

#define IMAGE_COUNT (sizeof(IMAGES) / sizeof(IMAGES[0]))

static lv_obj_t* s_image;
static lv_obj_t* s_image_name;
static size_t s_image_index;
static bool s_first_pass = true;

static void image_path(const char* name, char* path)
{
    snprintf(path, ASSET_PATH_MAX, "S:/spiffs/img/%s.bin", name);
}

static const lv_font_t* load_font(const char* name)
{
    char path[ASSET_PATH_MAX];
    snprintf(path, sizeof(path), "S:/spiffs/fonts/%s.fnt", name);

    lv_font_t* font = lv_font_load(path);
    if (font == NULL) {
        ESP_LOGE(TAG, "font %s not loaded; using the built-in font", path);

        return LV_FONT_DEFAULT;
    }

    ui_log_memory(name);

    return font;
}

static lv_obj_t* add_label(lv_obj_t* parent, const lv_font_t* font, lv_coord_t x, lv_coord_t y, const char* text)
{
    lv_obj_t* label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_pos(label, x, y);
    lv_label_set_text(label, text);

    return label;
}

static void add_image(lv_obj_t* parent, const char* name, lv_coord_t x, lv_coord_t y)
{
    char path[ASSET_PATH_MAX];
    image_path(name, path);

    lv_obj_t* image = lv_img_create(parent);
    lv_img_set_src(image, path);
    lv_obj_set_pos(image, x, y);
}

static void add_edge(lv_obj_t* parent, lv_coord_t x, lv_coord_t y, lv_coord_t w, lv_coord_t h, lv_color_t color)
{
    lv_obj_t* edge = lv_obj_create(parent);
    lv_obj_remove_style_all(edge);
    lv_obj_set_style_bg_opa(edge, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(edge, color, 0);
    lv_obj_set_pos(edge, x, y);
    lv_obj_set_size(edge, w, h);
}

static void show_image(size_t index)
{
    char path[ASSET_PATH_MAX];
    image_path(IMAGES[index], path);

    lv_img_header_t header;
    if (lv_img_decoder_get_info(path, &header) != LV_RES_OK) {
        ESP_LOGE(TAG, "image %s not readable", path);
        lv_img_set_src(s_image, LV_SYMBOL_WARNING);
        lv_label_set_text_fmt(s_image_name, "нет: %s", IMAGES[index]);

        return;
    }

    if (s_first_pass) {
        ESP_LOGI(
            TAG, "image %u/%u %s cf=%u %ux%u", index + 1, IMAGE_COUNT, IMAGES[index], header.cf, header.w, header.h);
    }

    lv_img_set_src(s_image, path);
    lv_label_set_text(s_image_name, IMAGES[index]);
}

static void show_next_image(lv_timer_t* timer)
{
    s_image_index = (s_image_index + 1) % IMAGE_COUNT;
    if (s_image_index == 0) {
        s_first_pass = false;
    }

    show_image(s_image_index);
}

void ui_bringup_create(lv_obj_t* screen)
{
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    const lv_font_t* digits_10 = load_font("sf_b_10");
    const lv_font_t* text_15 = load_font("sf_sb_15");
    load_font("sf_sb_30"); // Loaded for the heap measurement only.
    const lv_font_t* digits_50 = load_font("sf_sb_50");
    load_font("sf_sb_60"); // Loaded for the heap measurement only.

    const lv_coord_t width = lv_obj_get_width(screen);
    const lv_coord_t height = lv_obj_get_height(screen);

    // One-pixel edges in distinct colors show clipped or shifted panel offsets.
    add_edge(screen, 0, 0, width, 1, lv_palette_main(LV_PALETTE_RED));
    add_edge(screen, 0, height - 1, width, 1, lv_palette_main(LV_PALETTE_GREEN));
    add_edge(screen, 0, 0, 1, height, lv_palette_main(LV_PALETTE_BLUE));
    add_edge(screen, width - 1, 0, 1, height, lv_palette_main(LV_PALETTE_YELLOW));

    // Portrait 135x240, buttons below the glass. Overflowing content must clip, not
    // turn the screen into a scrollable container with a visible scrollbar.
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    add_label(screen, text_15, 4, 3, "Проверка");
    s_image_name = add_label(screen, text_15, 4, 20, "");
    lv_obj_set_width(s_image_name, width - 8);
    lv_label_set_long_mode(s_image_name, LV_LABEL_LONG_WRAP);

    s_image = lv_img_create(screen);
    lv_obj_align(s_image, LV_ALIGN_TOP_MID, 0, 56);

    add_label(screen, digits_50, 4, 158, "23°");
    add_image(screen, "status/good", 4, 208);
    add_image(screen, "batt/full_not_charging", 66, 217);
    add_label(screen, digits_10, 91, 216, "85%");

    show_image(s_image_index);
    lv_timer_create(show_next_image, IMAGE_PERIOD_MS, NULL);
    ui_log_memory("bring-up screen");
}
