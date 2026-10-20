#include "board_sleep.h"

#include <stdbool.h>
#include <stdint.h>

#include "board_pins.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PANEL_RESET_LOW_US 1000 // ST7789 needs at least 10 us.
#define PANEL_RESET_WAIT_MS 120 // Until the panel accepts Sleep In after reset.
#define RELEASE_STABLE_US 50000
#define RELEASE_TIMEOUT_US 3000000
#define RELEASE_POLL_MS 10

// Masks of buttons that stayed released, bit i for board_button_t i.
#define RELEASED(button) (1u << (button))
#define ALL_RELEASED ((1u << BOARD_BUTTON_COUNT) - 1)

static const char* TAG = "board_sleep";

static esp_err_t hold_rtc_low(gpio_num_t pin)
{
    ESP_RETURN_ON_ERROR(rtc_gpio_init(pin), TAG, "RTC mux");
    ESP_RETURN_ON_ERROR(rtc_gpio_set_level(pin, 0), TAG, "RTC LOW");
    ESP_RETURN_ON_ERROR(rtc_gpio_set_direction(pin, RTC_GPIO_MODE_OUTPUT_ONLY), TAG, "RTC output");
    ESP_RETURN_ON_ERROR(rtc_gpio_pullup_dis(pin), TAG, "pull-up off");
    ESP_RETURN_ON_ERROR(rtc_gpio_pulldown_dis(pin), TAG, "pull-down off");

    return rtc_gpio_hold_en(pin);
}

static esp_err_t reset_panel_into_sleep(void)
{
    ESP_RETURN_ON_ERROR(gpio_set_level(BOARD_PIN_PANEL_RESET, 0), TAG, "RST LOW");
    esp_rom_delay_us(PANEL_RESET_LOW_US);
    ESP_RETURN_ON_ERROR(gpio_set_level(BOARD_PIN_PANEL_RESET, 1), TAG, "RST HIGH");
    vTaskDelay(pdMS_TO_TICKS(PANEL_RESET_WAIT_MS) + 1); // +1: at least 120 ms near a tick boundary.
    ESP_RETURN_ON_ERROR(gpio_hold_en(BOARD_PIN_PANEL_RESET), TAG, "RST hold");
    gpio_deep_sleep_hold_en();

    return ESP_OK;
}

// A button counts as released once it has stayed released for RELEASE_STABLE_US.
static uint32_t wait_for_release(void)
{
    int64_t released_since[BOARD_BUTTON_COUNT] = {0};
    const int64_t deadline = esp_timer_get_time() + RELEASE_TIMEOUT_US;
    uint32_t released = 0;

    for (;;) {
        const int64_t now = esp_timer_get_time();
        released = 0;
        for (unsigned i = 0; i < BOARD_BUTTON_COUNT; i++) {
            if (gpio_get_level(board_button_pin((board_button_t)i)) == 0) {
                released_since[i] = 0;
            } else if (released_since[i] == 0) {
                released_since[i] = now;
            }

            if (released_since[i] != 0 && now - released_since[i] >= RELEASE_STABLE_US) {
                released |= RELEASED(i);
            }
        }

        if (released == ALL_RELEASED || now >= deadline) {
            return released;
        }

        vTaskDelay(pdMS_TO_TICKS(RELEASE_POLL_MS));
    }
}

static esp_err_t arm_wake(uint32_t released)
{
    ESP_RETURN_ON_ERROR(esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL), TAG, "clear wake");
    if (released & RELEASED(BOARD_BUTTON_PREV)) {
        ESP_RETURN_ON_ERROR(esp_sleep_enable_ext0_wakeup(board_button_pin(BOARD_BUTTON_PREV), 0), TAG, "EXT0 PREV");
    }

    // ALL_LOW with a single pin: either button wakes on its own.
    if (released & RELEASED(BOARD_BUTTON_NEXT)) {
        ESP_RETURN_ON_ERROR(
            esp_sleep_enable_ext1_wakeup_io(1ULL << board_button_pin(BOARD_BUTTON_NEXT), ESP_EXT1_WAKEUP_ALL_LOW),
            TAG,
            "EXT1 NEXT");
    }

    return ESP_OK;
}

void board_enter_deep_sleep(void* ctx)
{
    // Failures are logged and sleep goes on: staying awake would drain the battery.
    esp_err_t err = hold_rtc_low(BOARD_PIN_BACKLIGHT);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "backlight hold: %s", esp_err_to_name(err));
    }

    err = hold_rtc_low(BOARD_PIN_ADC_ENABLE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ADC_EN hold: %s", esp_err_to_name(err));
    }

    err = reset_panel_into_sleep();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "panel reset: %s", esp_err_to_name(err));
    }

    const uint32_t released = wait_for_release();
    err = arm_wake(released);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wake sources: %s", esp_err_to_name(err));
    }

    if (released != ALL_RELEASED) {
        ESP_LOGW(
            TAG,
            "button held for 3 s; wake by PREV=%d NEXT=%d",
            (released & RELEASED(BOARD_BUTTON_PREV)) != 0,
            (released & RELEASED(BOARD_BUTTON_NEXT)) != 0);
    }

    ESP_LOGI(TAG, "deep sleep");
    esp_deep_sleep_start();
}
