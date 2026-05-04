#pragma once

#include <stdbool.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

enum font_id {
    FONT_ID_SF_B_10,
    FONT_ID_SF_SB_15,
    FONT_ID_SF_SB_30,
    FONT_ID_SF_SB_50,
    FONT_ID_SF_SB_60,

    FONT_ID_COUNT,
};

bool ui_fonts_init(void);
const lv_font_t* font_get(enum font_id id);
void font_set(lv_obj_t* obj, enum font_id id, lv_style_selector_t selector);

#ifdef __cplusplus
}
#endif
