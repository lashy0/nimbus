#pragma once

#include <stdbool.h>
#include "esp_lcd_panel_ops.h"
#include "backlight.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    esp_lcd_panel_handle_t panel_handle;
    backlight_handle_t* bl_handle;
} power_manager_config_t;

void power_manager_init(const power_manager_config_t* config);

void power_manager_shutdown(void);
void power_manager_enter_monitoring(void);
void power_manager_exit_monitoring(void);

void power_manager_check_wakeup_reason(void);

bool power_manager_is_monitoring(void);

#ifdef __cplusplus
}
#endif
