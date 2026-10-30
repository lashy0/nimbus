#include "ui_font.h"

#include "ui_assets.h"

const lv_font_t* ui_font_bold_10;
const lv_font_t* ui_font_semibold_15;
const lv_font_t* ui_font_semibold_30;
const lv_font_t* ui_font_semibold_50;
const lv_font_t* ui_font_semibold_60;

static const lv_font_t* load(const char* path)
{
    const lv_font_t* font = lv_binfont_create(path);
    if (font == NULL) {
        LV_LOG_WARN("font %s not loaded", path);

        return LV_FONT_DEFAULT;
    }

    return font;
}

void ui_font_load(void)
{
    ui_font_bold_10 = load(UI_FONT("bold_10"));
    ui_font_semibold_15 = load(UI_FONT("semibold_15"));
    ui_font_semibold_30 = load(UI_FONT("semibold_30"));
    ui_font_semibold_50 = load(UI_FONT("semibold_50"));
    ui_font_semibold_60 = load(UI_FONT("semibold_60"));
}
