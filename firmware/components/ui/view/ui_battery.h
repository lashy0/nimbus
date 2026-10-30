#pragma once

#include <stdbool.h>

#include "power_types.h"

/**
 * @file
 * Battery indicator on LVGL's top layer, over every page.
 */

/** Creates the indicator showing an unknown battery. */
void ui_battery_create(void);

void ui_battery_update(const power_status_t* status);

void ui_battery_set_visible(bool visible);
