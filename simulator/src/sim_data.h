#pragma once

#include "power_types.h"
#include "sensing_types.h"

/**
 * @file
 * Made-up measurements and power, changed from the keyboard.
 */

void sim_data_get_snapshot(sensing_snapshot_t* out);

void sim_data_get_power(power_status_t* out);

/** Handles a data key: i, t, h, c, e, u or b; other keys are ignored. */
void sim_data_key(char key);
