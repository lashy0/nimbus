#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sensing_types.h"

// BSEC progress stays private; consumers receive the resulting phase.
typedef struct {
    sensing_snapshot_t snapshot;
    bool stabilized; // BSEC stabilization status reached 1.
    bool run_in;     // BSEC run-in status reached 1.
} sensing_outputs_t;

typedef struct {
    bool saved_calibrated; // A state with IAQ accuracy >= 1 is saved.
    bool saved_high;       // A state with IAQ accuracy 3 is saved.
    int64_t saved_ms;      // When storage last matched BSEC; INT64_MIN before that.
    int64_t attempt_ms;    // Last automatic save attempt; INT64_MIN before the first.
    bool record_unread;    // Storage could not be read at boot; its record may be good.
} sensing_save_policy_t;

// Called only for a complete, finite set; absent IAQ has accuracy 0.
sensing_phase_t sensing_policy_phase(const sensing_outputs_t* outputs);

// Returns the next backoff and advances a saturating count; reset it on success.
uint32_t sensing_policy_retry_delay(size_t* failures);

void sensing_policy_init(sensing_save_policy_t* policy);
void sensing_policy_saved(sensing_save_policy_t* policy, uint8_t accuracy, int64_t now_ms);

// A reason to checkpoint, or NULL. Records the attempt before I/O so failures
// and unchanged blobs observe the same minimum interval.
const char* sensing_policy_checkpoint(
    sensing_save_policy_t* policy,
    const sensing_snapshot_t* snapshot,
    int64_t now_ms);

// An unreadable record may be good; only a calibrated state replaces it.
bool sensing_policy_save_final(const sensing_save_policy_t* policy, const sensing_snapshot_t* snapshot);
