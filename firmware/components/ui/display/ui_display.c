#include "ui_display.h"

#include "esp_check.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui_render.h"

#define PANEL_SLEEP_GAP_US (120 * 1000) // ST7789: between Sleep In and Sleep Out either way.

static const char* TAG = "ui_display";

static const char* const STATE_NAMES[] = {
    [UI_DISPLAY_ON] = "ON",
    [UI_DISPLAY_DIM] = "DIM",
    [UI_DISPLAY_OFF] = "OFF",
};

static esp_pm_lock_handle_t s_pm_lock;
static esp_lcd_panel_handle_t s_panel;
static ui_backlight_fn s_set_backlight;
static ui_backlight_pin_fn s_release_backlight;
static ui_backlight_pin_fn s_reclaim_backlight;
static void* s_backlight_ctx;

// A failed transition keeps the completed steps so the next request continues.
typedef struct {
    ui_display_state_t state;
    bool settled;
    bool pm_held;
    bool panel_asleep;
    bool backlight_released;
    int64_t panel_sleep_changed_us;
    uint8_t brightness_percent;
} display_t;

static display_t s_display;

esp_err_t ui_display_init(const ui_config_t* config)
{
    ESP_RETURN_ON_FALSE(s_pm_lock == NULL, ESP_ERR_INVALID_STATE, TAG, "already initialized");

    ESP_RETURN_ON_ERROR(esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "display", &s_pm_lock), TAG, "PM lock");
    const esp_err_t err = esp_pm_lock_acquire(s_pm_lock);
    if (err != ESP_OK) {
        esp_pm_lock_delete(s_pm_lock);
        s_pm_lock = NULL;

        return err;
    }

    s_panel = config->panel;
    s_backlight_ctx = config->backlight_ctx;
    s_release_backlight = config->release_backlight;
    s_reclaim_backlight = config->reclaim_backlight;
    s_set_backlight = config->set_backlight;
    s_display.pm_held = true;
    s_display.panel_sleep_changed_us = esp_timer_get_time();

    return ESP_OK;
}

esp_err_t ui_display_start(uint8_t brightness_percent)
{
    const int64_t frame_started = esp_timer_get_time();
    ESP_RETURN_ON_ERROR(ui_render_draw_frame(), TAG, "first frame");

    // Panel RAM keeps the last frame through a reset: after deep sleep the shutdown
    // question was visible before the new frame. Display output starts only once
    // the new frame is in RAM.
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel, true), TAG, "display on");
    ESP_RETURN_ON_ERROR(s_set_backlight(s_backlight_ctx, brightness_percent), TAG, "backlight");

    // The panel left Sleep In during its init, shortly before.
    s_display = (display_t){
        .state = UI_DISPLAY_ON,
        .settled = true,
        .pm_held = true,
        .panel_sleep_changed_us = esp_timer_get_time(),
        .brightness_percent = brightness_percent,
    };

    ESP_LOGI(
        TAG, "first frame drawn in %lld us, backlight %u%%", esp_timer_get_time() - frame_started, brightness_percent);

    return ESP_OK;
}

static esp_err_t set_backlight(uint8_t percent)
{
    if (percent == s_display.brightness_percent) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(s_set_backlight(s_backlight_ctx, percent), TAG, "backlight %u%%", percent);
    s_display.brightness_percent = percent;

    return ESP_OK;
}

// Keeps the ST7789 gap between Sleep In and Sleep Out, which a quick wake after
// OFF would otherwise break.
static esp_err_t set_panel_sleep(bool sleep)
{
    if (s_display.panel_asleep == sleep) {
        return ESP_OK;
    }

    const int64_t wait_us = s_display.panel_sleep_changed_us + PANEL_SLEEP_GAP_US - esp_timer_get_time();
    if (wait_us > 0) {
        vTaskDelay(pdMS_TO_TICKS((wait_us + 999) / 1000) + 1);
    }

    const int64_t sent_us = esp_timer_get_time();
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_sleep(s_panel, sleep), TAG, "%s", sleep ? "Sleep In" : "Sleep Out");
    s_display.panel_sleep_changed_us = sent_us;
    s_display.panel_asleep = sleep;

    return ESP_OK;
}

esp_err_t ui_display_abort_init(bool renderer_stopped)
{
    if (s_pm_lock == NULL) {
        return ESP_OK;
    }

    // Use the callback even if startup failed before the cached brightness was set.
    ESP_RETURN_ON_ERROR(s_set_backlight(s_backlight_ctx, 0), TAG, "backlight off");
    s_display.brightness_percent = 0;
    ESP_RETURN_ON_FALSE(renderer_stopped, ESP_ERR_INVALID_STATE, TAG, "renderer cleanup not confirmed; PM lock kept");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel, false), TAG, "display off");
    ESP_RETURN_ON_ERROR(set_panel_sleep(true), TAG, "panel sleep");
    ESP_RETURN_ON_ERROR(s_release_backlight(s_backlight_ctx), TAG, "backlight release");
    s_display.backlight_released = true;

    if (s_display.pm_held) {
        ESP_RETURN_ON_ERROR(esp_pm_lock_release(s_pm_lock), TAG, "PM lock release");
        s_display.pm_held = false;
    }

    ESP_RETURN_ON_ERROR(esp_pm_lock_delete(s_pm_lock), TAG, "PM lock delete");
    s_pm_lock = NULL;
    s_panel = NULL;
    s_set_backlight = NULL;
    s_release_backlight = NULL;
    s_reclaim_backlight = NULL;
    s_backlight_ctx = NULL;
    s_display = (display_t){0};

    return ESP_OK;
}

static esp_err_t turn_off(void)
{
    // Nothing has changed yet if LVGL is busy.
    ESP_RETURN_ON_ERROR(ui_render_quiesce(), TAG, "stop LVGL");
    s_display.settled = false;

    if (!s_display.backlight_released) {
        ESP_RETURN_ON_ERROR(set_backlight(0), TAG, "backlight off");
    }

    if (!s_display.panel_asleep) {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel, false), TAG, "display off");
        ESP_RETURN_ON_ERROR(set_panel_sleep(true), TAG, "panel sleep");
    }

    if (!s_display.backlight_released) {
        ESP_RETURN_ON_ERROR(s_release_backlight(s_backlight_ctx), TAG, "backlight release");
        s_display.backlight_released = true;
    }

    // Only now, with the panel asleep and no transfer left, may the chip light-sleep.
    if (s_display.pm_held) {
        ESP_RETURN_ON_ERROR(esp_pm_lock_release(s_pm_lock), TAG, "PM lock release");
        s_display.pm_held = false;
    }

    s_display.state = UI_DISPLAY_OFF;
    s_display.settled = true;
    ESP_LOGI(TAG, "display OFF");

    return ESP_OK;
}

static esp_err_t wake(void)
{
    const int64_t started_us = esp_timer_get_time();
    s_display.settled = false;

    if (!s_display.pm_held) {
        ESP_RETURN_ON_ERROR(esp_pm_lock_acquire(s_pm_lock), TAG, "PM lock acquire");
        s_display.pm_held = true;
    }

    if (s_display.backlight_released) {
        ESP_RETURN_ON_ERROR(s_reclaim_backlight(s_backlight_ctx), TAG, "backlight reclaim");
        s_display.backlight_released = false;
        s_display.brightness_percent = 0;
    }

    ESP_RETURN_ON_ERROR(set_panel_sleep(false), TAG, "panel wake");
    ESP_RETURN_ON_ERROR(ui_render_draw_frame(), TAG, "first frame");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(s_panel, true), TAG, "display on");
    ESP_LOGI(TAG, "display woke in %lld us", esp_timer_get_time() - started_us);

    return ESP_OK;
}

static esp_err_t turn_on(ui_display_state_t state, uint8_t brightness_percent)
{
    if (!s_display.settled || s_display.state == UI_DISPLAY_OFF) {
        const esp_err_t err = wake();
        if (err != ESP_OK) {
            // Dark is the safe state while the panel may show nothing or garbage.
            if (!s_display.backlight_released) {
                set_backlight(0);
            }

            return err;
        }
    }

    ESP_RETURN_ON_ERROR(set_backlight(brightness_percent), TAG, "backlight");
    if (state != s_display.state || !s_display.settled) {
        ESP_LOGI(TAG, "display %s at %u%%", STATE_NAMES[state], brightness_percent);
    }

    s_display.state = state;
    s_display.settled = true;

    return ESP_OK;
}

esp_err_t ui_display_set_state(ui_display_state_t state, uint8_t brightness_percent)
{
    // A recursive mutex can only be released by the task that holds it.
    ESP_RETURN_ON_FALSE(ui_render_owned_by_caller(), ESP_ERR_INVALID_STATE, TAG, "display is OFF for another task");

    if (state == UI_DISPLAY_OFF) {
        return s_display.settled && s_display.state == UI_DISPLAY_OFF ? ESP_OK : turn_off();
    }

    return turn_on(state, brightness_percent);
}
