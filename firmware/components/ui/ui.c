#include "ui.h"

#include "esp_check.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "buttons.h"
#include "sensing.h"
#include "ui_settings.h"
#include "ui_view.h"

#define UPDATE_PERIOD_MS 1000
#define BUTTONS_PERIOD_MS 20
#define SAVE_DELAY_MS 2000 // After the last change.

static const char* TAG = "ui";

static lv_display_t* s_display;
static esp_lcd_panel_handle_t s_panel;
static uint8_t s_brightness; // On the backlight; changed only in the LVGL task.
static lv_timer_t* s_save_timer;

// Runs in the LVGL task once the first frame is drawn. The panel sends the
// command only after the pending pixel data.
static void on_first_frame(lv_event_t* event)
{
    lv_display_remove_event_cb_with_user_data(s_display, on_first_frame, NULL);

    esp_err_t err = esp_lcd_panel_disp_on_off(s_panel, true);
    if (err == ESP_OK) {
        err = display_set_brightness(s_brightness);
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "screen on: %s", esp_err_to_name(err));
    }
}

// Runs in the LVGL task.
static void on_update(lv_timer_t* timer)
{
    sensing_snapshot_t snapshot;
    sensing_get_snapshot(&snapshot);
    ui_view_update(&snapshot);
}

// Runs in the LVGL task.
static void on_save(lv_timer_t* timer)
{
    lv_timer_pause(timer);

    esp_err_t err = ui_settings_save_brightness(s_brightness);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "brightness not saved: %s", esp_err_to_name(err));
    }
}

// Runs in the LVGL task.
static void on_buttons(lv_timer_t* timer)
{
    buttons_event_t event;
    while (buttons_receive(&event)) {
        ui_view_press(&event);
    }

    const uint8_t brightness = ui_view_brightness();
    if (brightness == s_brightness) {
        return;
    }

    s_brightness = brightness;
    esp_err_t err = display_set_brightness(brightness);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "brightness: %s", esp_err_to_name(err));
    }

    lv_timer_reset(s_save_timer);
    lv_timer_resume(s_save_timer);
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

    s_panel = display->panel;
    const uint8_t saved_brightness = load_brightness();

    // Drawing happens in the LVGL task, on its own stack.
    lvgl_port_lock(0);
    ui_view_create(saved_brightness);
    s_brightness = ui_view_brightness();
    lv_timer_create(on_update, UPDATE_PERIOD_MS, NULL);
    lv_timer_create(on_buttons, BUTTONS_PERIOD_MS, NULL);
    s_save_timer = lv_timer_create(on_save, SAVE_DELAY_MS, NULL);
    lv_timer_pause(s_save_timer);
    lv_display_add_event_cb(s_display, on_first_frame, LV_EVENT_REFR_READY, NULL);
    lvgl_port_unlock();

    return ESP_OK;
}
