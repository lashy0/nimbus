#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "power_types.h"

/**
 * @file
 * Pure battery calculations, private to power; no I/O, blocking or internal locking.
 * Callers own the estimator, serialize access and pass non-NULL pointers.
 */

// Initial thresholds for a single Li-ion cell, chosen before any measurement on
// battery power alone; revisit them with a reference voltmeter.
#define POWER_EXTERNAL_MIN_MV 4400  // A cell stays below 4.2 V; GPIO34 read 4.7 V on USB.
#define POWER_PLAUSIBLE_MIN_MV 2500 // Readings outside this range are a fault, not a level.
#define POWER_PLAUSIBLE_MAX_MV 6000
#define POWER_CRITICAL_MV 3300
#define POWER_CRITICAL_READINGS 3 // Consecutive low readings that confirm a discharge.
#define POWER_STARTUP_MIN_MV 3400
#define POWER_STARTUP_AFTER_BROWNOUT_MV 3500 // Above the start threshold: a brownout repeats otherwise.

typedef struct {
    power_status_t status;
    int32_t filtered_mv_x16; // Battery voltage filter, 0 when not running.
    uint8_t low_readings;
} power_estimator_t;

/** Clears the estimator to UNKNOWN with no valid percentage or confirmed discharge. */
void power_estimator_init(power_estimator_t* estimator);

/**
 * Feeds one reading in millivolts; set ok to false on read failure.
 * A failed or implausible reading invalidates the percentage and resets the
 * voltage filter, retaining the last plausible voltage and discharge evidence.
 */
void power_estimator_update(power_estimator_t* estimator, bool ok, uint32_t battery_mv);

/** Returns a 0..100% estimate from the generic Li-ion curve, clamped at its endpoints. */
uint8_t power_percent_from_mv(uint32_t battery_mv);

/**
 * Returns whether the reading in millivolts allows display and sensor startup.
 * An unknown voltage is allowed except after a brownout, which needs a margin
 * above the normal start threshold. Does not modify an estimator.
 */
bool power_startup_allowed(bool ok, uint32_t battery_mv, bool after_brownout);
