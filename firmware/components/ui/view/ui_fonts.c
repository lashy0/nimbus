#include "ui_fonts.h"

#include <stdio.h>

#include "ui_assets.h"

#define FONT_PATH_MAX 48

static const lv_font_t* load_font(const char* name)
{
    char path[FONT_PATH_MAX];
    snprintf(path, sizeof(path), UI_ASSET_ROOT "/fonts/%s.fnt", name);

    lv_font_t* font = lv_binfont_create(path);
    if (font == NULL) {
        LV_LOG_ERROR("font %s not loaded; using the built-in font", path);

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

static void unload_font(const lv_font_t* font)
{
    if (font != NULL && font != LV_FONT_DEFAULT) {
        lv_binfont_destroy((lv_font_t*)font);
    }
}

void ui_fonts_unload(ui_fonts_t* fonts)
{
    unload_font(fonts->digits_10);
    unload_font(fonts->text_15);
    unload_font(fonts->text_30);
    unload_font(fonts->digits_50);
    unload_font(fonts->digits_60);
    *fonts = (ui_fonts_t){0};
}
