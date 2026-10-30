#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    POWER_SOURCE_UNKNOWN = 0, /**< No plausible reading yet, or the battery cannot be read. */
    POWER_SOURCE_BATTERY,
    POWER_SOURCE_EXTERNAL, /**< USB; says nothing about charging. */
} power_source_t;

/** Latest battery estimate. */
typedef struct {
    power_source_t source;
    bool percent_valid;  /**< Only on BATTERY. */
    uint8_t percent;     /**< 0..100. */
    uint16_t battery_mv; /**< Last plausible reading, 0 before one; on EXTERNAL the charger rail. */
    bool critical;       /**< Confirmed discharge: the device has to power off. */
} power_status_t;
