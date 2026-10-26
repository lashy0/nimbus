#include "sensing_policy.h"

#define CHECKPOINT_PERIOD_MS (6LL * 60 * 60 * 1000) // Saves accumulated calibration without frequent flash writes.
#define MIN_SAVE_INTERVAL_MS 60000                  // Limits flash wear when saves fail or repeat.
#define NO_TIME INT64_MIN

static const uint32_t BACKOFF_MS[] = {1000, 2000, 5000, 10000, 30000};

#define BACKOFF_COUNT (sizeof(BACKOFF_MS) / sizeof(BACKOFF_MS[0]))

sensing_phase_t sensing_policy_phase(const sensing_outputs_t* outputs)
{
    const sensing_snapshot_t* snapshot = &outputs->snapshot;
    if (!outputs->stabilized || !outputs->run_in || snapshot->iaq_accuracy == 0) {
        return SENSING_PHASE_WARMUP;
    }

    return snapshot->iaq_accuracy == 1 ? SENSING_PHASE_CALIBRATING : SENSING_PHASE_READY;
}

uint32_t sensing_policy_retry_delay(size_t* failures)
{
    const uint32_t delay_ms = BACKOFF_MS[*failures];
    if (*failures < BACKOFF_COUNT - 1) {
        (*failures)++;
    }

    return delay_ms;
}

void sensing_policy_init(sensing_save_policy_t* policy)
{
    *policy = (sensing_save_policy_t){.saved_ms = NO_TIME, .attempt_ms = NO_TIME};
}

void sensing_policy_saved(sensing_save_policy_t* policy, uint8_t accuracy, int64_t now_ms)
{
    policy->saved_calibrated |= accuracy >= 1;
    policy->saved_high |= accuracy == 3;
    policy->saved_ms = now_ms;
}

static bool calibrated(const sensing_snapshot_t* snapshot)
{
    return snapshot->phase == SENSING_PHASE_CALIBRATING || snapshot->phase == SENSING_PHASE_READY;
}

// Only a usable result is worth saving: stabilized, run in and IAQ accuracy >= 1.
static const char* save_reason(const sensing_save_policy_t* policy, const sensing_snapshot_t* snapshot, int64_t now_ms)
{
    if (!calibrated(snapshot)) {
        return NULL;
    }

    if (!policy->saved_calibrated) {
        return "first calibrated result";
    }

    if (snapshot->iaq_accuracy == 3 && !policy->saved_high) {
        return "first high accuracy";
    }

    if (now_ms - policy->saved_ms >= CHECKPOINT_PERIOD_MS) {
        return "periodic";
    }

    return NULL;
}

const char* sensing_policy_checkpoint(sensing_save_policy_t* policy, const sensing_snapshot_t* snapshot, int64_t now_ms)
{
    const char* reason = save_reason(policy, snapshot, now_ms);
    if (reason == NULL || (policy->attempt_ms != NO_TIME && now_ms - policy->attempt_ms < MIN_SAVE_INTERVAL_MS)) {
        return NULL;
    }

    policy->attempt_ms = now_ms;

    return reason;
}

bool sensing_policy_save_final(const sensing_save_policy_t* policy, const sensing_snapshot_t* snapshot)
{
    return !policy->record_unread || calibrated(snapshot);
}
