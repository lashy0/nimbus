#pragma once

#include "buttons.h"
#include "backlight.h"
#include "display.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    display_handles_t* display;
    backlight_handle_t* backlight;
} app_config_t;

void app_init(const app_config_t* config);

void app_on_button_short_press(button_id_t btn_id);
void app_on_button_long_press(button_id_t btn_id);

#ifdef __cplusplus
}
#endif
