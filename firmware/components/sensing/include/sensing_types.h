#pragma once

#include <stdint.h>

/** How to read the values of a snapshot. */
typedef enum {
    SENSING_STATUS_NO_DATA = 0, /**< No measurement yet; the values are zero. */
    SENSING_STATUS_OK,          /**< The values come from the latest measurement. */
    SENSING_STATUS_ERROR,       /**< The latest measurement failed; the values are from an earlier one. */
} sensing_status_t;

/** Latest measurement published by the sensing task. */
typedef struct {
    sensing_status_t status;
    float temperature_c;    /**< Corrected for the sensor's self-heating. */
    float humidity_percent; /**< 0..100, corrected for the sensor's self-heating. */
    float pressure_hpa;
    float iaq;            /**< Indoor air quality index, 0..500. */
    uint8_t iaq_accuracy; /**< BSEC calibration of iaq: 0 stabilizing, 1 low, 2 medium, 3 high. */
} sensing_snapshot_t;
