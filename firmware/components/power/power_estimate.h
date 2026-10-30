#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "power_types.h"

/**
 * @file
 * Turns battery readings into a status. No I/O.
 */

typedef struct {
    power_status_t status;
    int32_t filtered_mv_x256; // 0 while the filter is not running.
    uint8_t low_readings;     // In a row, below the critical voltage.
} power_estimate_t;

void power_estimate_init(power_estimate_t* estimate);

/** Feeds one reading; ok is false after a failed read. */
void power_estimate_update(power_estimate_t* estimate, bool ok, uint32_t battery_mv);

/** Whether the supply can start the display and the sensor; an unknown one can, but not after a brownout. */
bool power_estimate_startup_allowed(const power_status_t* status, bool after_brownout);
