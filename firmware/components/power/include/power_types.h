#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    POWER_SOURCE_UNKNOWN,  /**< No plausible reading. */
    POWER_SOURCE_BATTERY,  /**< Plausible reading below the external supply threshold. */
    POWER_SOURCE_EXTERNAL, /**< Inferred from the voltage; says nothing about charging. */
} power_source_t;

/**
 * Battery estimate from the latest poll; inspect source and percent_valid before use.
 * battery_mv may retain an earlier plausible reading while source is UNKNOWN.
 */
typedef struct {
    power_source_t source;
    uint16_t battery_mv; /**< Last plausible reading; on EXTERNAL the charger rail. */
    bool percent_valid;  /**< Only on BATTERY. */
    uint8_t percent;     /**< Smoothed estimate from a generic Li-ion curve. */
    bool critical;       /**< Confirmed discharge: the device has to power off. */
} power_status_t;
