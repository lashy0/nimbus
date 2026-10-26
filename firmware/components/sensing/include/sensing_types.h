#pragma once

#include <stdint.h>

/** Measurement values are available from WARMUP on; IAQ only from CALIBRATING on. */
typedef enum {
    SENSING_PHASE_INITIALIZING, /**< Waiting for setup and the first complete measurement. */
    SENSING_PHASE_ERROR,        /**< Setup failed, measurement failed or data expired; retry scheduled. */
    SENSING_PHASE_WARMUP,       /**< Temperature, humidity and pressure available; IAQ not usable yet. */
    SENSING_PHASE_CALIBRATING,  /**< Stabilized and run in, IAQ accuracy 1. */
    SENSING_PHASE_READY,        /**< Stabilized and run in, IAQ accuracy 2 or 3. */
} sensing_phase_t;

/**
 * Cached sensing result; phase alone determines which values are usable.
 * Working phases contain complete, finite, fresh values when read through
 * sensing_get_snapshot(). INITIALIZING and ERROR carry no usable values.
 * A copied snapshot does not track freshness after it is returned to the caller.
 */
typedef struct {
    sensing_phase_t phase;

    float iaq;            /**< Adaptive IAQ for mobile use, 0..500. */
    uint8_t iaq_accuracy; /**< Accuracy of BSEC_OUTPUT_IAQ, 0..3. */

    float temperature_c;    /**< Heat-compensated. */
    float humidity_percent; /**< Heat-compensated. */
    float pressure_pa;
} sensing_snapshot_t;
