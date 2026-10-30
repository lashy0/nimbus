#include "sim_port.h"

#include <stdbool.h>

#include "lvgl.h"

#include "sim_log.h"
#include "ui_port.h"

// Shade opacity per missing backlight percent; a lit screen never looks fully black.
#define SHADE_OPA_PER_PERCENT 2

static lv_obj_t* s_shade;
static bool s_off;

void sim_port_create(void)
{
    s_shade = lv_obj_create(lv_layer_sys());
    lv_obj_remove_style_all(s_shade);
    lv_obj_set_size(s_shade, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(s_shade, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_shade, LV_OPA_COVER, 0);
}

void sim_port_power_off(void)
{
    s_off = true;
    lv_obj_set_style_bg_opa(s_shade, LV_OPA_COVER, 0);
    sim_log("port", "powered off");
}

void ui_port_set_backlight(uint8_t percent)
{
    if (s_off) {
        return;
    }

    const lv_opa_t opa = percent == 0 ? LV_OPA_COVER : (lv_opa_t)((100 - percent) * SHADE_OPA_PER_PERCENT);
    lv_obj_set_style_bg_opa(s_shade, opa, 0);
    sim_log("port", "backlight %u %%", percent);
}

void ui_port_save_brightness(uint8_t percent)
{
    sim_log("port", "brightness %u %% saved", percent);
}
