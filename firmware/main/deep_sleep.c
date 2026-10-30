#include "deep_sleep.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "board.h"
#include "power.h"

#define PANEL_RESET_LOW_US 1000 // At least 10 us.

#define RELEASE_POLL_MS 10
#define RELEASE_STABLE_POLLS 5    // 50 ms.
#define RELEASE_TIMEOUT_POLLS 300 // 3 s.

typedef struct {
    bool valid;
    uint8_t reason;
    uint16_t battery_mv;
} sleep_record_t;

static const char* TAG = "deep_sleep";

// Zeroed on every boot but a wake from deep sleep.
static RTC_DATA_ATTR sleep_record_t s_record;

static const char* reason_name(uint8_t reason)
{
    static const char* const NAMES[] = {
        [DEEP_SLEEP_USER] = "power-off",
        [DEEP_SLEEP_CRITICAL] = "critical battery",
        [DEEP_SLEEP_LOW_START] = "low voltage at start",
    };

    return reason < sizeof(NAMES) / sizeof(NAMES[0]) ? NAMES[reason] : "unknown";
}

static void log_failure(const char* what, esp_err_t err)
{
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s: %s", what, esp_err_to_name(err));
    }
}

// LOW on the digital output first, so the pin does not float on the way.
static esp_err_t release_low(gpio_num_t pin)
{
    ESP_RETURN_ON_ERROR(gpio_set_level(pin, 0), TAG, "LOW");
    ESP_RETURN_ON_ERROR(gpio_set_direction(pin, GPIO_MODE_OUTPUT), TAG, "output");
    ESP_RETURN_ON_ERROR(rtc_gpio_hold_dis(pin), TAG, "hold");

    return rtc_gpio_deinit(pin);
}

void deep_sleep_restore(void)
{
    if (s_record.valid) {
        ESP_LOGI(
            TAG,
            "woke after %s at %u mV, causes 0x%" PRIx32,
            reason_name(s_record.reason),
            s_record.battery_mv,
            esp_sleep_get_wakeup_causes());
        s_record.valid = false;
    }

    log_failure("backlight release", release_low(BOARD_PIN_LCD_BACKLIGHT));
    log_failure("ADC_EN release", release_low(BOARD_PIN_ADC_ENABLE));
    log_failure("panel reset release", gpio_hold_dis(BOARD_PIN_LCD_RESET));
    gpio_deep_sleep_hold_dis();

    // Wake pins stay on the RTC mux.
    log_failure("PREV release", rtc_gpio_deinit(BOARD_PIN_BUTTON_PREV));
    log_failure("NEXT release", rtc_gpio_deinit(BOARD_PIN_BUTTON_NEXT));
}

// On the RTC mux, so the digital side can no longer drive the pin.
static esp_err_t hold_low(gpio_num_t pin)
{
    ESP_RETURN_ON_ERROR(rtc_gpio_init(pin), TAG, "RTC mux");
    ESP_RETURN_ON_ERROR(rtc_gpio_set_level(pin, 0), TAG, "LOW");
    ESP_RETURN_ON_ERROR(rtc_gpio_set_direction(pin, RTC_GPIO_MODE_OUTPUT_ONLY), TAG, "output");

    return rtc_gpio_hold_en(pin);
}

// A hardware reset leaves the panel in Sleep In with the display off.
static esp_err_t reset_panel(void)
{
    ESP_RETURN_ON_ERROR(gpio_set_direction(BOARD_PIN_LCD_RESET, GPIO_MODE_OUTPUT), TAG, "output");
    ESP_RETURN_ON_ERROR(gpio_set_level(BOARD_PIN_LCD_RESET, 0), TAG, "LOW");
    esp_rom_delay_us(PANEL_RESET_LOW_US);
    ESP_RETURN_ON_ERROR(gpio_set_level(BOARD_PIN_LCD_RESET, 1), TAG, "HIGH");
    ESP_RETURN_ON_ERROR(gpio_hold_en(BOARD_PIN_LCD_RESET), TAG, "hold");
    gpio_deep_sleep_hold_en();

    return ESP_OK;
}

static bool stays_released(gpio_num_t pin, uint32_t* high_polls)
{
    *high_polls = gpio_get_level(pin) ? *high_polls + 1 : 0;

    return *high_polls >= RELEASE_STABLE_POLLS;
}

static esp_err_t arm_wake(void)
{
    // The buttons enable GPIO wake for light sleep; in deep sleep it wakes at once.
    ESP_RETURN_ON_ERROR(esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL), TAG, "clear");
    ESP_RETURN_ON_ERROR(gpio_set_direction(BOARD_PIN_BUTTON_PREV, GPIO_MODE_INPUT), TAG, "PREV input");
    ESP_RETURN_ON_ERROR(gpio_set_direction(BOARD_PIN_BUTTON_NEXT, GPIO_MODE_INPUT), TAG, "NEXT input");

    uint32_t prev_polls = 0;
    uint32_t next_polls = 0;
    bool prev_released = false;
    bool next_released = false;
    for (int i = 0; i < RELEASE_TIMEOUT_POLLS; i++) {
        prev_released = stays_released(BOARD_PIN_BUTTON_PREV, &prev_polls);
        next_released = stays_released(BOARD_PIN_BUTTON_NEXT, &next_polls);
        if (prev_released && next_released) {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(RELEASE_POLL_MS));
    }

    if (!prev_released || !next_released) {
        ESP_LOGW(TAG, "button held; wake by PREV %d, NEXT %d", prev_released, next_released);
    }

    if (prev_released) {
        ESP_RETURN_ON_ERROR(esp_sleep_enable_ext0_wakeup(BOARD_PIN_BUTTON_PREV, 0), TAG, "EXT0");
    }

    // ALL_LOW with a single pin wakes on that pin alone.
    if (next_released) {
        ESP_RETURN_ON_ERROR(
            esp_sleep_enable_ext1_wakeup_io(1ULL << BOARD_PIN_BUTTON_NEXT, ESP_EXT1_WAKEUP_ALL_LOW), TAG, "EXT1");
    }

    return ESP_OK;
}

void deep_sleep_enter(deep_sleep_reason_t reason)
{
    power_status_t power;
    power_get_status(&power);
    s_record = (sleep_record_t){.valid = true, .reason = (uint8_t)reason, .battery_mv = power.battery_mv};

    log_failure("backlight hold", hold_low(BOARD_PIN_LCD_BACKLIGHT));
    log_failure("ADC_EN hold", hold_low(BOARD_PIN_ADC_ENABLE));
    log_failure("panel reset", reset_panel());
    log_failure("wake", arm_wake());

    ESP_LOGI(TAG, "%s at %u mV", reason_name(reason), power.battery_mv);
    esp_deep_sleep_start();
}
