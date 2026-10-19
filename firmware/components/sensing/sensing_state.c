#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "sensing_internal.h"

#define CHECKPOINT_PERIOD_MS (6LL * 60 * 60 * 1000) // Bosch's example saves every 6 hours.
#define MIN_SAVE_INTERVAL_MS 60000                  // Limits flash wear when saves fail or repeat.
#define NO_TIME INT64_MIN

static const char* TAG = "sensing_state";

static void mark_saved(sensing_checkpoint_t* checkpoint, uint8_t accuracy, int64_t now_ms)
{
    checkpoint->saved_calibrated |= accuracy >= 1;
    checkpoint->saved_high |= accuracy == 3;
    checkpoint->saved_ms = now_ms;
}

esp_err_t sensing_state_restore(void* instance, const storage_bsec_key_t* key, sensing_checkpoint_t* checkpoint)
{
    *checkpoint = (sensing_checkpoint_t){.saved_ms = NO_TIME, .attempt_ms = NO_TIME};

    size_t length = 0;
    uint8_t accuracy = 0;
    esp_err_t err = storage_load_bsec_state(key, checkpoint->state, sizeof(checkpoint->state), &length, &accuracy);
    if (err != ESP_OK) {
        // Nothing usable is kept: a mismatching record is replaced by the first save.
        if (err == ESP_ERR_NOT_FOUND) {
            ESP_LOGI(TAG, "no saved state; calibration starts over");
        } else {
            ESP_LOGW(TAG, "saved state not used: %s; calibration starts over", esp_err_to_name(err));
        }

        return err;
    }

    uint8_t* work = malloc(BSEC_MAX_WORKBUFFER_SIZE);
    ESP_RETURN_ON_FALSE(work != NULL, ESP_ERR_NO_MEM, TAG, "work buffer");
    const bsec_library_return_t rc =
        bsec_set_state_m(instance, checkpoint->state, length, work, BSEC_MAX_WORKBUFFER_SIZE);
    free(work);
    if (rc != BSEC_OK) {
        ESP_LOGW(TAG, "BSEC rejected the saved state: %d; calibration starts over", rc);

        return ESP_FAIL;
    }

    // The saved milestones are not saved again; the period counts from boot.
    checkpoint->length = length;
    mark_saved(checkpoint, accuracy, esp_timer_get_time() / 1000);
    ESP_LOGI(TAG, "restored state: %u bytes, saved at IAQ accuracy %u", length, accuracy);

    return ESP_OK;
}

// Only a usable result is worth saving: stabilized, run in and IAQ accuracy >= 1.
static const char* save_reason(
    const sensing_checkpoint_t* checkpoint,
    const sensing_snapshot_t* snapshot,
    int64_t now_ms)
{
    if (snapshot->phase != SENSING_PHASE_CALIBRATING && snapshot->phase != SENSING_PHASE_READY) {
        return NULL;
    }

    if (!checkpoint->saved_calibrated) {
        return "first calibrated result";
    }

    if (snapshot->iaq_accuracy == 3 && !checkpoint->saved_high) {
        return "first high accuracy";
    }

    if (now_ms - checkpoint->saved_ms >= CHECKPOINT_PERIOD_MS) {
        return "periodic";
    }

    return NULL;
}

void sensing_state_checkpoint(
    const sensing_bsec_t* bsec,
    const sensing_snapshot_t* snapshot,
    sensing_checkpoint_t* checkpoint,
    int64_t now_ms)
{
    const char* reason = save_reason(checkpoint, snapshot, now_ms);
    if (reason == NULL ||
        (checkpoint->attempt_ms != NO_TIME && now_ms - checkpoint->attempt_ms < MIN_SAVE_INTERVAL_MS)) {
        return;
    }

    checkpoint->attempt_ms = now_ms;

    uint8_t state[BSEC_MAX_STATE_BLOB_SIZE];
    uint32_t length = 0;
    uint8_t* work = malloc(BSEC_MAX_WORKBUFFER_SIZE);
    if (work == NULL) {
        ESP_LOGW(TAG, "no memory to read the state");

        return;
    }

    const bsec_library_return_t rc =
        bsec_get_state_m(bsec->instance, 0, state, sizeof(state), work, BSEC_MAX_WORKBUFFER_SIZE, &length);
    free(work);
    if (rc != BSEC_OK) {
        ESP_LOGW(TAG, "get state: %d", rc);

        return;
    }

    if (length == checkpoint->length && memcmp(state, checkpoint->state, length) == 0) {
        mark_saved(checkpoint, snapshot->iaq_accuracy, now_ms);

        return;
    }

    // On failure the previous record stays and the milestone is retried later.
    const esp_err_t err = storage_save_bsec_state(&bsec->key, state, length, snapshot->iaq_accuracy);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "save failed (%s): %s", reason, esp_err_to_name(err));

        return;
    }

    memcpy(checkpoint->state, state, length);
    checkpoint->length = length;
    mark_saved(checkpoint, snapshot->iaq_accuracy, now_ms);
    ESP_LOGI(TAG, "saved state (%s): %lu bytes, IAQ accuracy %u", reason, length, snapshot->iaq_accuracy);
}
