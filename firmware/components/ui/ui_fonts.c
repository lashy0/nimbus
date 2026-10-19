#include <stdio.h>

#include "esp_log.h"
#include "ui_internal.h"

#define FONT_PATH_MAX 48

static const char* TAG = "ui_fonts";

static const lv_font_t* load_font(const char* name)
{
    char path[FONT_PATH_MAX];
    snprintf(path, sizeof(path), "S:/spiffs/fonts/%s.fnt", name);

    lv_font_t* font = lv_font_load(path);
    if (font == NULL) {
        ESP_LOGE(TAG, "font %s not loaded; using the built-in font", path);

        return LV_FONT_DEFAULT;
    }

    return font;
}

void ui_fonts_load(ui_fonts_t* fonts)
{
    fonts->digits_10 = load_font("sf_b_10");
    fonts->text_15 = load_font("sf_sb_15");
    fonts->text_30 = load_font("sf_sb_30");
    fonts->digits_50 = load_font("sf_sb_50");
    fonts->digits_60 = load_font("sf_sb_60");
}
