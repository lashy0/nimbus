#pragma once

#include <stdbool.h>
#include <stdint.h>

// After this many panic, watchdog or brownout resets the boot stops and the device
// sleeps: firmware that crashes on every start would otherwise restart forever.
#define RESTART_CRASH_LIMIT 3

typedef enum {
    RESTART_POWER_ON, // Power applied or the EN button: a new series starts.
    RESTART_CRASH,    // Panic, a watchdog or brownout.
    RESTART_OTHER,    // Deep-sleep wake or software reset: the series goes on.
} restart_cause_t;

/**
 * Restart counter for retained memory that survives resets and deep sleep.
 * The magic and inverted count distinguish a valid record from leftover contents.
 */
typedef struct {
    uint32_t magic;
    uint32_t crashes;
    uint32_t crashes_inverted;
} restart_record_t;

/**
 * Counts this reset into the non-NULL caller-owned record.
 * Pure transition; callers serialize access to the same record.
 * @return true for a crash reaching RESTART_CRASH_LIMIT, requiring boot to stop.
 * Other reset causes allow boot, so a button wake gives one more attempt.
 */
bool restart_guard_on_boot(restart_record_t* record, restart_cause_t cause);

/** Clears the non-NULL caller-owned record after a stable run; serialize access. */
void restart_guard_clear(restart_record_t* record);
