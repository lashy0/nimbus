#include "restart_guard.h"

#define RECORD_MAGIC 0x4e425247u
#define CRASHES_MAX 255

static bool record_valid(const restart_record_t* record)
{
    return record->magic == RECORD_MAGIC && record->crashes == ~record->crashes_inverted &&
           record->crashes <= CRASHES_MAX;
}

static void store(restart_record_t* record, uint32_t crashes)
{
    record->magic = RECORD_MAGIC;
    record->crashes = crashes;
    record->crashes_inverted = ~crashes;
}

bool restart_guard_on_boot(restart_record_t* record, restart_cause_t cause)
{
    // Memory is not trusted after a power loss, even when its contents look valid.
    uint32_t crashes = cause != RESTART_POWER_ON && record_valid(record) ? record->crashes : 0;
    if (cause == RESTART_CRASH && crashes < CRASHES_MAX) {
        crashes++;
    }

    store(record, crashes);

    return cause == RESTART_CRASH && crashes >= RESTART_CRASH_LIMIT;
}

void restart_guard_clear(restart_record_t* record)
{
    store(record, 0);
}
