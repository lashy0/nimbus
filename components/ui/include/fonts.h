#pragma once

#include <stdbool.h>
#include <lvgl.h>

#define FONT_PATH(name) "S:/spiffs/fonts/" name ".fnt"

#define FONT_SF_B_10  FONT_PATH("sf_b_10")
#define FONT_SF_SB_15 FONT_PATH("sf_sb_15")
#define FONT_SF_SB_30 FONT_PATH("sf_sb_30")
#define FONT_SF_SB_50 FONT_PATH("sf_sb_50")
#define FONT_SF_SB_60 FONT_PATH("sf_sb_60")

typedef struct {
    const char* path;
    lv_font_t* font;
    bool attempted;
} font_info_t;

#ifdef __cplusplus
extern "C" {
#endif

extern font_info_t FONT_INFO_SF_B_10;
extern font_info_t FONT_INFO_SF_SB_15;
extern font_info_t FONT_INFO_SF_SB_30;
extern font_info_t FONT_INFO_SF_SB_50;
extern font_info_t FONT_INFO_SF_SB_60;

bool ui_fonts_init(void);
const lv_font_t* font_get(const font_info_t* info);
void font_set(lv_obj_t* obj, const font_info_t* info, lv_style_selector_t selector);

#ifdef __cplusplus
}
#endif
