#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "backlight.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    esp_lcd_panel_handle_t panel_handle;
    backlight_handle_t* bl_handle;
} power_manager_config_t;

typedef struct {
    int percent;
    bool charging;
    uint16_t voltage_mv;
    bool valid;
} power_battery_info_t;

void power_manager_init(const power_manager_config_t* config);

void power_manager_shutdown(void);
void power_manager_enter_monitoring(void);
void power_manager_exit_monitoring(void);

void power_manager_check_wakeup_reason(void);
esp_err_t power_manager_read_battery(power_battery_info_t* out_info);
esp_err_t power_manager_set_active_brightness(uint8_t brightness_percent, bool persist);
esp_err_t power_manager_step_active_brightness(void);
uint8_t power_manager_get_active_brightness(void);

bool power_manager_is_monitoring(void);

#ifdef __cplusplus
}
#endif
