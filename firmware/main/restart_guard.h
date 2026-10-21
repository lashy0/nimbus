#pragma once

#include <stdbool.h>
#include <stdint.h>

// After this many panic or watchdog resets in a row the boot stops and the device
// sleeps: firmware that crashes on every start would otherwise restart forever.
#define RESTART_CRASH_LIMIT 3

typedef enum {
    RESTART_POWER_ON, // Power applied or the EN button: a new series starts.
    RESTART_CRASH,    // Panic or a watchdog.
    RESTART_OTHER,    // Deep-sleep wake, brownout or software reset: the series goes on.
} restart_cause_t;

// Kept in memory that survives resets and deep sleep but not a power loss; the
// inverted copy tells a kept record from leftover contents.
typedef struct {
    uint32_t magic;
    uint32_t crashes;
    uint32_t crashes_inverted;
} restart_record_t;

// Counts this reset into the record. Returns true when the boot must stop: a crash
// that reached RESTART_CRASH_LIMIT. Any other reset boots, so a button press after
// that sleep gives one more attempt.
bool restart_guard_on_boot(restart_record_t* record, restart_cause_t cause);

// Ends the series after a stable run.
void restart_guard_clear(restart_record_t* record);
