#include "ui.h"

#include "esp_check.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

#define BUFFER_LINES 24
#define BRIGHTNESS_PERCENT 100

static const char* TAG = "ui";

static lv_display_t* s_display;

static void build_screen(lv_display_t* display)
{
    lv_obj_t* screen = lv_display_get_screen_active(display);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);

    lv_obj_t* label = lv_label_create(screen);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_label_set_text(label, "Nimbus");
    lv_obj_center(label);
}

// Under the LVGL lock: the port task flushes through the same panel IO.
static esp_err_t show_first_frame(esp_lcd_panel_handle_t panel)
{
    lvgl_port_lock(0);

    build_screen(s_display);
    lv_refr_now(s_display);
    const esp_err_t err = esp_lcd_panel_disp_on_off(panel, true);

    lvgl_port_unlock();

    return err;
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

    esp_err_t ret = ESP_OK;
    s_display = lvgl_port_add_disp(&display_config);
    ESP_GOTO_ON_FALSE(s_display != NULL, ESP_ERR_NO_MEM, fail, TAG, "LVGL display");
    ESP_GOTO_ON_ERROR(show_first_frame(display->panel), fail, TAG, "first frame");
    ESP_GOTO_ON_ERROR(display_set_brightness(BRIGHTNESS_PERCENT), fail, TAG, "backlight");

    return ESP_OK;

fail:
    if (s_display != NULL) {
        lvgl_port_remove_disp(s_display);
        s_display = NULL;
    }
    lvgl_port_deinit();

    return ret;
}
