#pragma once

#include <lvgl.h>
#include <stdbool.h>
#include "screens.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_init();

void ui_tick();

void ui_update_iaq(int value);

void ui_update_temp(int value);

void ui_update_hum(int value);

void ui_update_battery(int percent, bool charging);

void ui_switch_next(void);

void ui_switch_prev(void);

void loadScreen(enum ScreensEnum screenId);

#ifdef __cplusplus
}
#endif