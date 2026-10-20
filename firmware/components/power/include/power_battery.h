#pragma once

#include <stdbool.h>
#include <stdint.h>

// Initial thresholds for a single Li-ion cell, chosen before any measurement on
// battery power alone; revisit them with a reference voltmeter.
#define POWER_EXTERNAL_MIN_MV 4400  // A cell stays below 4.2 V; GPIO34 read 4.7 V on USB.
#define POWER_PLAUSIBLE_MIN_MV 2500 // Readings outside this range are a fault, not a level.
#define POWER_PLAUSIBLE_MAX_MV 6000
#define POWER_CRITICAL_MV 3300
#define POWER_CRITICAL_READINGS 3 // Consecutive low readings that confirm a discharge.
#define POWER_STARTUP_MIN_MV 3400
#define POWER_STARTUP_AFTER_BROWNOUT_MV 3500 // Above the start threshold: a brownout repeats otherwise.

typedef enum {
    POWER_SOURCE_UNKNOWN,  // No plausible reading.
    POWER_SOURCE_BATTERY,  // Below POWER_EXTERNAL_MIN_MV.
    POWER_SOURCE_EXTERNAL, // Inferred from the voltage; says nothing about charging.
} power_source_t;

typedef struct {
    power_source_t source;
    uint16_t battery_mv; // Last plausible reading; on EXTERNAL the charger rail.
    bool percent_valid;  // Only on BATTERY.
    uint8_t percent;     // Smoothed estimate from a generic Li-ion curve.
    bool critical;       // Confirmed discharge: the device has to power off.
} power_status_t;

typedef struct {
    power_status_t status;
    int32_t filtered_mv_x16; // Battery voltage filter, 0 when not running.
    uint8_t low_readings;
} power_estimator_t;

void power_estimator_init(power_estimator_t* estimator);

// Feeds one reading; ok is false when the read failed.
void power_estimator_update(power_estimator_t* estimator, bool ok, uint32_t battery_mv);

uint8_t power_percent_from_mv(uint32_t battery_mv);

// Whether a start may switch on the display and sensor. An unknown voltage is
// allowed except after a brownout, which needs a margin above the start threshold.
bool power_startup_allowed(bool ok, uint32_t battery_mv, bool after_brownout);
