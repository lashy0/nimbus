#pragma once

#include <stdbool.h>
#include "power_manager.h"

extern power_manager_config_t s_pm_config;
extern bool s_is_monitoring;

void pm_battery_init(void);
void pm_brightness_init(void);
void pm_brightness_apply_current(void);
