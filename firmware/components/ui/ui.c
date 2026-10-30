#include "ui.h"

#include "esp_check.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "sensing.h"
#include "ui_view.h"

#define BUFFER_LINES 24
#define BRIGHTNESS_PERCENT 100
#define UPDATE_PERIOD_MS 1000

static const char* TAG = "ui";

static lv_display_t* s_display;
static esp_lcd_panel_handle_t s_panel;

// Runs in the LVGL task once the first frame is drawn. The panel sends the
// command only after the pending pixel data.
static void on_first_frame(lv_event_t* event)
{
    lv_display_remove_event_cb_with_user_data(s_display, on_first_frame, NULL);

    esp_err_t err = esp_lcd_panel_disp_on_off(s_panel, true);
    if (err == ESP_OK) {
        err = display_set_brightness(BRIGHTNESS_PERCENT);
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

esp_err_t ui_start(const display_t* display, BaseType_t core)
{
    ESP_RETURN_ON_FALSE(s_display == NULL, ESP_ERR_INVALID_STATE, TAG, "already started");

    lvgl_port_cfg_t port_config = ESP_LVGL_PORT_INIT_CONFIG();
    port_config.task_affinity = core == tskNO_AFFINITY ? -1 : core;
    ESP_RETURN_ON_ERROR(lvgl_port_init(&port_config), TAG, "LVGL port");

    const lvgl_port_display_cfg_t display_config = {
        .io_handle = display->io,
        .panel_handle = display->panel,
        .buffer_size = DISPLAY_WIDTH * BUFFER_LINES,
        .double_buffer = true,
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

    // Drawing happens in the LVGL task, on its own stack.
    lvgl_port_lock(0);
    ui_view_create();
    lv_timer_create(on_update, UPDATE_PERIOD_MS, NULL);
    lv_display_add_event_cb(s_display, on_first_frame, LV_EVENT_REFR_READY, NULL);
    lvgl_port_unlock();

    return ESP_OK;
}
