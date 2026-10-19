#pragma once

#include <stdbool.h>
#include <stdint.h>

// Phase rules are applied in this order; IAQ is shown only from CALIBRATING on.
typedef enum {
    SENSING_PHASE_INITIALIZING, // Sensor or BSEC not set up yet.
    SENSING_PHASE_ERROR,        // Set-up or a measurement failed; a retry is scheduled.
    SENSING_PHASE_WARMUP,       // No stabilization/run-in yet, or IAQ accuracy 0.
    SENSING_PHASE_CALIBRATING,  // Stabilized and run in, IAQ accuracy 1.
    SENSING_PHASE_READY,        // Stabilized and run in, IAQ accuracy 2 or 3.
} sensing_phase_t;

// Latest BSEC outputs. A value is valid only if BSEC returned it since the sensor
// last (re)started; updated_ms tells how fresh it is.
typedef struct {
    sensing_phase_t phase;
    uint32_t generation; // Incremented with every new set of outputs.
    int64_t updated_ms;  // Monotonic time of the newest outputs; 0 before the first.

    bool iaq_valid;
    float static_iaq;
    uint8_t iaq_accuracy; // Accuracy of BSEC_OUTPUT_IAQ, 0..3.

    bool temperature_valid;
    float temperature_c; // Heat-compensated.

    bool humidity_valid;
    float humidity_percent; // Heat-compensated.

    bool pressure_valid;
    float pressure_pa;

    bool stabilized; // BSEC stabilization status reached 1.
    bool run_in;     // BSEC run-in status reached 1.
} sensing_snapshot_t;
