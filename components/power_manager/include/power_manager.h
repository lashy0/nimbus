#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "backlight.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Power manager dependencies.
 */
typedef struct {
    /**< LCD panel handle used for display power transitions. */
    esp_lcd_panel_handle_t panel_handle;
    /**< Backlight handle used for brightness and display off/on. */
    backlight_handle_t* bl_handle;
} power_manager_config_t;

/**
 * @brief Battery telemetry snapshot.
 */
typedef struct {
    /**< Battery level in percent, -1 when unknown/invalid. */
    int percent;
    /**< Heuristic charging state inferred from voltage trend. */
    bool charging;
    /**< Filtered battery voltage in millivolts. */
    uint16_t voltage_mv;
    /**< True when battery reading is considered valid. */
    bool valid;
} power_battery_info_t;

/**
 * @brief Initialize power manager runtime.
 *
 * @param[in] config Pointer to platform handles required by power manager.
 */
void power_manager_init(const power_manager_config_t* config);

/**
 * @brief Show shutdown screen and enter deep sleep.
 */
void power_manager_shutdown(void);

/**
 * @brief Enter low-power monitoring mode (display off, reduced activity).
 */
void power_manager_enter_monitoring(void);

/**
 * @brief Exit monitoring mode and restore active display state.
 */
void power_manager_exit_monitoring(void);

/**
 * @brief Log wakeup reason from the last sleep cycle.
 */
void power_manager_check_wakeup_reason(void);

/**
 * @brief Read battery state from ADC and infer charging trend.
 *
 * @param[out] out_info Output battery information.
 *
 * @return
 * - ESP_OK: read completed (check @ref power_battery_info_t::valid).
 * - ESP_ERR_INVALID_ARG: @p out_info is NULL.
 * - ESP_ERR_INVALID_STATE: battery ADC is not initialized.
 * - Other ESP error from GPIO/ADC operations.
 */
esp_err_t power_manager_read_battery(power_battery_info_t* out_info);

/**
 * @brief Set active brightness level.
 *
 * @param[in] brightness_percent Brightness value in percent [5..100].
 * @param[in] persist Persist value in NVS when true.
 *
 * @return
 * - ESP_OK: brightness updated.
 * - ESP_ERR_INVALID_ARG: brightness is outside valid range.
 */
esp_err_t power_manager_set_active_brightness(uint8_t brightness_percent, bool persist);

/**
 * @brief Move brightness to the next preset value and persist it.
 *
 * Presets cycle through predefined levels.
 *
 * @return ESP_OK on success or an error from @ref power_manager_set_active_brightness.
 */
esp_err_t power_manager_step_active_brightness(void);

/**
 * @brief Get current active brightness level.
 *
 * @return Brightness percent in range [5..100].
 */
uint8_t power_manager_get_active_brightness(void);

/**
 * @brief Check whether monitoring mode is active.
 *
 * @return true when in monitoring mode, false otherwise.
 */
bool power_manager_is_monitoring(void);

#ifdef __cplusplus
}
#endif
