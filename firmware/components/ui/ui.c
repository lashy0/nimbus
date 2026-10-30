#include "ui.h"

#include <stdatomic.h>

#include "esp_check.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_pm.h"
#include "esp_random.h"
#include "lvgl.h"

#include "buttons.h"
#include "power.h"
#include "sensing.h"
#include "ui_app.h"
#include "ui_pause.h"
#include "ui_port.h"
#include "ui_settings.h"
#include "ui_view.h"

#define SENSING_PERIOD_MS 1000
#define POWER_PERIOD_MS 250
#define BUTTONS_PERIOD_MS 20

static const char* TAG = "ui";

static lv_display_t* s_display;
static esp_lcd_panel_handle_t s_panel;
static uint8_t s_backlight;            // Percent on the backlight; 0 while dark.
static esp_pm_lock_handle_t s_pm_lock; // Held while the screen is lit: the backlight PWM stops in light sleep.
static bool s_stopped;
static atomic_bool s_power_off_requested;

static void hold_awake(bool awake)
{
    const esp_err_t err = awake ? esp_pm_lock_acquire(s_pm_lock) : esp_pm_lock_release(s_pm_lock);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "PM lock: %s", esp_err_to_name(err));
    }
}

// Runs in the LVGL task once a frame is drawn. The panel sends the command
// only after the pending pixel data.
static void on_frame_drawn(lv_event_t* event)
{
    lv_display_remove_event_cb_with_user_data(s_display, on_frame_drawn, NULL);
    if (s_backlight == 0) {
        return;
    }

    esp_err_t err = esp_lcd_panel_disp_on_off(s_panel, true);
    if (err == ESP_OK) {
        err = display_set_brightness(s_backlight);
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "screen on: %s", esp_err_to_name(err));
    }
}

void ui_port_set_backlight(uint8_t percent)
{
    const bool was_dark = s_backlight == 0;
    s_backlight = percent;

    esp_err_t err = ESP_OK;
    if (percent == 0) {
        err = display_set_brightness(0);
        if (err == ESP_OK) {
            err = esp_lcd_panel_disp_on_off(s_panel, false);
        }
        hold_awake(false);
    } else if (was_dark) {
        // The panel keeps its last frame with the output off, so a new one is drawn first.
        hold_awake(true);
        lv_obj_invalidate(lv_screen_active());
        lv_display_add_event_cb(s_display, on_frame_drawn, LV_EVENT_REFR_READY, NULL);
    } else {
        err = display_set_brightness(percent);
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "backlight: %s", esp_err_to_name(err));
    }
}

void ui_port_save_brightness(uint8_t percent)
{
    const esp_err_t err = ui_settings_save_brightness(percent);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "brightness not saved: %s", esp_err_to_name(err));
    }
}

// Runs in the LVGL task.
static void on_sensing(lv_timer_t* timer)
{
    sensing_snapshot_t snapshot;
    sensing_get_snapshot(&snapshot);
    ui_view_update(&snapshot);
}

// Runs in the LVGL task.
static void on_power(lv_timer_t* timer)
{
    power_status_t status;
    power_get_status(&status);
    ui_app_update_power(&status);

    if (s_backlight == 0) {
        ui_pause_request();
    }
}

// Runs in the LVGL task.
static void on_buttons(lv_timer_t* timer)
{
    buttons_event_t event;
    while (buttons_receive(&event)) {
        ui_app_press(&event);
    }

    if (ui_view_power_off_confirmed()) {
        atomic_store(&s_power_off_requested, true);
    }
}

static uint8_t load_brightness(void)
{
    uint8_t brightness = 0;
    esp_err_t err = ui_settings_load_brightness(&brightness);
    if (err != ESP_OK && err != ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "saved brightness: %s", esp_err_to_name(err));
    }

    return brightness;
}

esp_err_t ui_start(const display_t* display, BaseType_t core)
{
    ESP_RETURN_ON_FALSE(s_display == NULL, ESP_ERR_INVALID_STATE, TAG, "already started");
    ESP_RETURN_ON_ERROR(esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "ui", &s_pm_lock), TAG, "PM lock");

    lvgl_port_cfg_t port_config = ESP_LVGL_PORT_INIT_CONFIG();
    port_config.task_affinity = core == tskNO_AFFINITY ? -1 : core;
    ESP_RETURN_ON_ERROR(lvgl_port_init(&port_config), TAG, "LVGL port");

    const lvgl_port_display_cfg_t display_config = {
        .io_handle = display->io,
        .panel_handle = display->panel,
        .buffer_size = DISPLAY_WIDTH * DISPLAY_HEIGHT,
        .double_buffer = false,
        .hres = DISPLAY_WIDTH,
        .vres = DISPLAY_HEIGHT,
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags.buff_dma = true,
        .flags.swap_bytes = true, // The panel reads RGB565 high byte first.
    };

    s_display = lvgl_port_add_disp(&display_config);
    if (s_display == NULL) {
        lvgl_port_deinit();
        ESP_LOGE(TAG, "LVGL display");

        return ESP_ERR_NO_MEM;
    }

    ESP_RETURN_ON_ERROR(ui_pause_start(core), TAG, "pause");
    s_panel = display->panel;
    const uint8_t saved_brightness = load_brightness();

    // Drawing happens in the LVGL task, on its own stack.
    lvgl_port_lock(0);
    lv_rand_set_seed(esp_random());
    ui_app_create(saved_brightness);
    lv_timer_create(on_sensing, SENSING_PERIOD_MS, NULL);
    lv_timer_create(on_power, POWER_PERIOD_MS, NULL);
    lv_timer_create(on_buttons, BUTTONS_PERIOD_MS, NULL);
    lvgl_port_unlock();

    return ESP_OK;
}

bool ui_power_off_requested(void)
{
    return atomic_load(&s_power_off_requested);
}

esp_err_t ui_stop(void)
{
    ESP_RETURN_ON_FALSE(s_display != NULL && !s_stopped, ESP_ERR_INVALID_STATE, TAG, "not running");

    ui_pause_disable();
    lvgl_port_lock(0);
    lvgl_port_stop();
    s_stopped = true;

    ui_app_save_pending();

    esp_err_t err = display_set_brightness(0);
    if (err == ESP_OK) {
        err = esp_lcd_panel_disp_on_off(s_panel, false);
    }
    if (err == ESP_OK) {
        err = esp_lcd_panel_disp_sleep(s_panel, true);
    }
    lvgl_port_unlock();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "screen off: %s", esp_err_to_name(err));
    }

    return err;
}
