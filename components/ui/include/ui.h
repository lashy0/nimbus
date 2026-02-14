#pragma once

#include <lvgl.h>
#include <stdbool.h>
#include "screens.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_init();
void ui_finish_startup(bool has_non_critical_error);
void ui_tick();

void ui_update_iaq(int value);
void ui_update_temp(int value);
void ui_update_hum(int value);
void ui_update_calibration_status(bool stabilization_done, bool run_in_done);
void ui_update_battery(int percent, bool charging);

void ui_switch_next(void);
void ui_switch_prev(void);
void loadScreen(enum ScreensEnum screenId);
enum ScreensEnum ui_get_current_screen(void);

void ui_show_start(void);
void ui_show_charging(void);
void ui_show_no_charging(void);
void ui_show_calibration(void);

void ui_show_question(const char* text, void (*on_yes)(void), void (*on_no)(void), bool select_yes);
void ui_question_select_yes(void);
void ui_question_select_no(void);
void ui_question_confirm(void);
void ui_hide_special(void);

#ifdef __cplusplus
}
#endif
