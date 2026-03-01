#pragma once

#include <lvgl.h>
#include <stdbool.h>
#include <stdint.h>
#include "screens.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize UI subsystem and show startup screen.
 */
void ui_init();

/**
 * @brief Finalize startup transition and move to runtime screens.
 *
 * @param[in] has_non_critical_error If true, startup warning state is shown.
 */
void ui_finish_startup(bool has_non_critical_error);

/**
 * @brief UI periodic tick hook (reserved for future use).
 */
void ui_tick();

/**
 * @brief Update IAQ value in UI state and active IAQ screen.
 *
 * @param[in] value IAQ value (typically 0..500).
 */
void ui_update_iaq(int value);

/**
 * @brief Update temperature value in UI state and active temperature screen.
 *
 * @param[in] value Temperature value in Celsius degrees.
 */
void ui_update_temp(int value);

/**
 * @brief Update humidity value in UI state and active humidity screen.
 *
 * @param[in] value Relative humidity in percent.
 */
void ui_update_hum(int value);

/**
 * @brief Update calibration flags shown on IAQ warmup status.
 *
 * @param[in] stabilization_done Stabilization status.
 * @param[in] run_in_done Run-in status.
 */
void ui_update_calibration_status(bool stabilization_done, bool run_in_done);

/**
 * @brief Update IAQ quality status for runtime warmup indicators.
 *
 * @param[in] iaq_accuracy BSEC IAQ accuracy (0..3).
 * @param[in] stabilization_done Stabilization status.
 * @param[in] run_in_done Run-in status.
 */
void ui_update_iaq_quality(uint8_t iaq_accuracy, bool stabilization_done, bool run_in_done);

/**
 * @brief Update battery state shown in status bar.
 *
 * @param[in] percent Battery level percentage, negative value means unknown.
 * @param[in] charging Charging state flag.
 */
void ui_update_battery(int percent, bool charging);

/**
 * @brief Switch to the next regular data screen.
 */
void ui_switch_next(void);

/**
 * @brief Switch to the previous regular data screen.
 */
void ui_switch_prev(void);

/**
 * @brief Load target regular screen.
 *
 * @param[in] screenId Target screen identifier.
 */
void loadScreen(enum ScreensEnum screenId);

/**
 * @brief Get currently active screen identifier.
 *
 * @return Current screen id.
 */
enum ScreensEnum ui_get_current_screen(void);

/**
 * @brief Show startup screen.
 */
void ui_show_start(void);

/**
 * @brief Show charging splash screen.
 */
void ui_show_charging(void);

/**
 * @brief Show no-charging/dead screen.
 */
void ui_show_no_charging(void);

/**
 * @brief Show brightness setup screen.
 *
 * @param[in] value_percent Initial brightness value in percent.
 */
void ui_show_brightness(uint8_t value_percent);

/**
 * @brief Update brightness value on brightness screen.
 *
 * @param[in] value_percent Brightness value in percent.
 */
void ui_update_brightness_value(uint8_t value_percent);

/**
 * @brief Show confirmation question screen.
 *
 * @param[in] text Question text.
 * @param[in] on_yes Callback invoked when user confirms YES.
 * @param[in] on_no Callback invoked when user confirms NO.
 * @param[in] select_yes Initial selected answer (true = YES).
 */
void ui_show_question(const char* text, void (*on_yes)(void), void (*on_no)(void), bool select_yes);

/**
 * @brief Select YES option on question screen.
 */
void ui_question_select_yes(void);

/**
 * @brief Select NO option on question screen.
 */
void ui_question_select_no(void);

/**
 * @brief Confirm selected answer on question screen and invoke callback.
 */
void ui_question_confirm(void);

/**
 * @brief Hide special screen and return to previous regular screen.
 */
void ui_hide_special(void);

#ifdef __cplusplus
}
#endif
