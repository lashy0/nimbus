#include "crash_guard.h"

#include <inttypes.h>
#include <stdint.h>

#include "esp_attr.h"
#include "esp_core_dump.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"

#define CRASH_LIMIT 3
#define STABLE_RUN_US (5LL * 60 * 1000 * 1000)
#define RECORD_MAGIC 0x4e494d42 // "NIMB"

typedef struct {
    uint32_t magic;
    uint32_t crashes;
    uint32_t crashes_inverted; // ~crashes; RTC memory holds garbage after a power-on.
} crash_record_t;

static const char* TAG = "crash_guard";

// Kept through every reset but a power-on.
static RTC_NOINIT_ATTR crash_record_t s_record;

static bool s_cleared;

static const char* crash_name(esp_reset_reason_t reason)
{
    switch (reason) {
        case ESP_RST_PANIC:
            return "panic";
        case ESP_RST_INT_WDT:
            return "interrupt watchdog";
        case ESP_RST_TASK_WDT:
            return "task watchdog";
        case ESP_RST_WDT:
            return "watchdog";
        case ESP_RST_BROWNOUT:
            return "brownout";
        default:
            return NULL;
    }
}

static void store(uint32_t crashes)
{
    s_record = (crash_record_t){.magic = RECORD_MAGIC, .crashes = crashes, .crashes_inverted = ~crashes};
}

static uint32_t stored_crashes(void)
{
    const bool valid = s_record.magic == RECORD_MAGIC && s_record.crashes == ~s_record.crashes_inverted;

    return valid ? s_record.crashes : 0;
}

// Earlier dumps stay in flash, so it is read only after a reset that writes one.
static void log_core_dump(void)
{
    static esp_core_dump_summary_t summary;
    if (esp_core_dump_get_summary(&summary) != ESP_OK) {
        return;
    }

    char reason[128] = "";
    esp_core_dump_get_panic_reason(reason, sizeof(reason));
    ESP_LOGE(TAG, "crash in task %s at 0x%08" PRIx32 ": %s", summary.exc_task, summary.exc_pc, reason);
}

bool crash_guard_check(void)
{
    const esp_reset_reason_t reason = esp_reset_reason();
    uint32_t crashes = reason == ESP_RST_POWERON ? 0 : stored_crashes();

    const char* crash = crash_name(reason);
    if (crash == NULL) {
        store(crashes);

        return false;
    }

    if (reason == ESP_RST_PANIC || reason == ESP_RST_INT_WDT || reason == ESP_RST_TASK_WDT) {
        log_core_dump();
    }

    if (crashes < UINT32_MAX) {
        crashes++;
    }
    store(crashes);

    ESP_LOGW(TAG, "%s, %" PRIu32 " in a row", crash, crashes);

    return crashes >= CRASH_LIMIT;
}

void crash_guard_poll(void)
{
    if (!s_cleared && esp_timer_get_time() >= STABLE_RUN_US) {
        s_cleared = true;
        store(0);
    }
}
