#include "fonts.h"

#include "esp_log.h"

static const char* TAG = "ui_fonts";

font_info_t FONT_INFO_SF_B_10 = {
    .path = FONT_SF_B_10,
    .font = NULL,
    .attempted = false,
};

font_info_t FONT_INFO_SF_SB_15 = {
    .path = FONT_SF_SB_15,
    .font = NULL,
    .attempted = false,
};

font_info_t FONT_INFO_SF_SB_30 = {
    .path = FONT_SF_SB_30,
    .font = NULL,
    .attempted = false,
};

font_info_t FONT_INFO_SF_SB_50 = {
    .path = FONT_SF_SB_50,
    .font = NULL,
    .attempted = false,
};

font_info_t FONT_INFO_SF_SB_60 = {
    .path = FONT_SF_SB_60,
    .font = NULL,
    .attempted = false,
};

static const lv_font_t* font_load(font_info_t* info, bool* out_ok)
{
    if (!info) {
        if (out_ok) {
            *out_ok = false;
        }
        return NULL;
    }

    if (info->attempted) {
        return info->font;
    }

    info->attempted = true;
    info->font = lv_font_load(info->path);
    if (info->font) {
        ESP_LOGI(TAG, "Loaded font: %s", info->path);
        return info->font;
    }

    ESP_LOGE(TAG, "Failed to load font: %s", info->path);
    if (out_ok) {
        *out_ok = false;
    }
    return NULL;
}

const lv_font_t* font_get(const font_info_t* info)
{
    if (!info) {
        return NULL;
    }
    return info->font;
}

void font_set(lv_obj_t* obj, const font_info_t* info, lv_style_selector_t selector)
{
    if (!obj) {
        return;
    }

    const lv_font_t* font = font_get(info);
    if (!font) {
        return;
    }

    lv_obj_set_style_text_font(obj, font, selector);
}

bool ui_fonts_init(void)
{
    static bool initialized = false;
    static bool init_ok = false;
    if (initialized) {
        return init_ok;
    }

    bool ok = true;
    font_load(&FONT_INFO_SF_B_10, &ok);
    font_load(&FONT_INFO_SF_SB_15, &ok);
    font_load(&FONT_INFO_SF_SB_30, &ok);
    font_load(&FONT_INFO_SF_SB_50, &ok);
    font_load(&FONT_INFO_SF_SB_60, &ok);

    initialized = true;
    init_ok = ok;
    return init_ok;
}
