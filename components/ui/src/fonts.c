#include "fonts.h"

#include "esp_log.h"

static const char* TAG = "ui_fonts";

#define FONT_PATH(name) "S:/spiffs/fonts/" name ".fnt"

static const char* const font_paths[FONT_ID_COUNT] = {
    [FONT_ID_SF_B_10]  = FONT_PATH("sf_b_10"),
    [FONT_ID_SF_SB_15] = FONT_PATH("sf_sb_15"),
    [FONT_ID_SF_SB_30] = FONT_PATH("sf_sb_30"),
    [FONT_ID_SF_SB_50] = FONT_PATH("sf_sb_50"),
    [FONT_ID_SF_SB_60] = FONT_PATH("sf_sb_60"),
};

static lv_font_t* fonts[FONT_ID_COUNT] = {0};

bool ui_fonts_init(void)
{
    static bool initialized = false;
    static bool init_ok = false;
    if (initialized) {
        return init_ok;
    }

    bool ok = true;
    for (int i = 0; i < FONT_ID_COUNT; ++i) {
        fonts[i] = lv_font_load(font_paths[i]);
        if (fonts[i]) {
            ESP_LOGI(TAG, "Loaded font: %s", font_paths[i]);
        } else {
            ESP_LOGE(TAG, "Failed to load font: %s", font_paths[i]);
            ok = false;
        }
    }

    initialized = true;
    init_ok = ok;
    return init_ok;
}

const lv_font_t* font_get(enum font_id id)
{
    if (id < 0 || id >= FONT_ID_COUNT) {
        return NULL;
    }
    return fonts[id];
}

void font_set(lv_obj_t* obj, enum font_id id, lv_style_selector_t selector)
{
    if (!obj) {
        return;
    }

    const lv_font_t* font = font_get(id);
    if (!font) {
        return;
    }

    lv_obj_set_style_text_font(obj, font, selector);
}
