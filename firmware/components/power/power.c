#include "power.h"

#include "esp_attr.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_sleep.h"

static const char* TAG = "power";

// RTC_DATA_ATTR is zeroed on every boot except a wake from deep sleep, so a valid
// record always describes the sleep this boot woke from.
typedef struct {
    bool valid;
    uint8_t reason;
    uint8_t source;
    uint16_t battery_mv;
} sleep_record_t;

static RTC_DATA_ATTR sleep_record_t s_sleep_record;

static power_config_t s_config;
static power_estimator_t s_estimator;

static const char* reason_name(uint8_t reason)
{
    static const char* const NAMES[] = {"user", "critical battery", "low voltage at start"};

    return reason < sizeof(NAMES) / sizeof(NAMES[0]) ? NAMES[reason] : "unknown";
}

static const char* source_name(power_source_t source)
{
    static const char* const NAMES[] = {"unknown", "battery", "external"};

    return NAMES[source];
}

esp_err_t power_init(const power_config_t* config)
{
    ESP_RETURN_ON_FALSE(
        config != NULL && config->read_battery != NULL && config->enter_sleep != NULL,
        ESP_ERR_INVALID_ARG,
        TAG,
        "config");

    s_config = *config;
    power_estimator_init(&s_estimator);

    if (s_sleep_record.valid) {
        ESP_LOGI(
            TAG,
            "woke from sleep: reason %s, %u mV (%s)",
            reason_name(s_sleep_record.reason),
            s_sleep_record.battery_mv,
            source_name(s_sleep_record.source));
        s_sleep_record.valid = false;
    }

    return ESP_OK;
}

static bool read_battery(uint32_t* battery_mv)
{
    const esp_err_t err = s_config.read_battery(s_config.read_ctx, battery_mv);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "battery read failed: %s", esp_err_to_name(err));
    }

    return err == ESP_OK;
}

void power_poll(power_status_t* out)
{
    uint32_t battery_mv = 0;
    const bool ok = read_battery(&battery_mv);
    const power_status_t before = s_estimator.status;
    power_estimator_update(&s_estimator, ok, battery_mv);
    *out = s_estimator.status;

    if (out->source != before.source || out->critical != before.critical) {
        ESP_LOGI(
            TAG,
            "source %s, %u mV, %d%%%s",
            source_name(out->source),
            out->battery_mv,
            out->percent_valid ? out->percent : -1,
            out->critical ? ", critical" : "");
    }
}

bool power_check_startup(bool after_brownout)
{
    uint32_t battery_mv = 0;
    const bool ok = read_battery(&battery_mv);
    power_estimator_update(&s_estimator, ok, battery_mv);

    const bool allowed = power_startup_allowed(ok, battery_mv, after_brownout);
    const char* brownout = after_brownout ? " after brownout" : "";
    const char* verdict = allowed ? "allowed" : "refused";
    if (ok) {
        ESP_LOGI(TAG, "start check: %lu mV%s, %s", battery_mv, brownout, verdict);
    } else {
        ESP_LOGW(TAG, "start check: voltage unknown%s, %s", brownout, verdict);
    }

    return allowed;
}

void power_enter_sleep(power_sleep_reason_t reason)
{
    s_sleep_record = (sleep_record_t){
        .valid = true,
        .reason = (uint8_t)reason,
        .source = (uint8_t)s_estimator.status.source,
        .battery_mv = s_estimator.status.battery_mv,
    };
    ESP_LOGI(TAG, "sleep: %s", reason_name(reason));

    s_config.enter_sleep(s_config.sleep_ctx);

    // The callback never returns; sleep without its pin preparation if it did.
    esp_deep_sleep_start();
}
