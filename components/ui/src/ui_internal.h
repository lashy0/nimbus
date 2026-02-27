#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ui.h"

extern int current_iaq;
extern uint8_t current_iaq_accuracy;
extern int current_temp;
extern int current_hum;
extern int current_batt_pct;
extern bool current_batt_charging;
extern bool current_stabilization_done;
extern bool current_run_in_done;
extern uint8_t current_brightness_pct;
extern const uint8_t UI_BRIGHTNESS_MIN_PCT;

extern enum ScreensEnum currentScreenId;
extern enum ScreensEnum previousScreenId;
extern bool startup_non_critical_error;
extern lv_obj_t* startup_error_icon;

extern void (*question_on_yes)(void);
extern void (*question_on_no)(void);
extern bool question_selected_yes;

void ui_apply_current_values(void);
void ui_apply_brightness_value(void);
void ui_apply_calibration_status_text(void);
void ui_apply_current_battery_status(void);
