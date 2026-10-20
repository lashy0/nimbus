#include "ui.h"

#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_pm.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui_internal.h"

#define LOCK_TIMEOUT_MS 1000
#define APPLY_LOCK_TIMEOUT_MS 100
#define FIRST_FLUSH_TIMEOUT_US (500 * 1000)

static const char* TAG = "ui";

static esp_pm_lock_handle_t s_pm_lock;
static lv_disp_t* s_disp;
static ui_fonts_t s_fonts;
static ui_backlight_fn s_set_backlight; // Set once the display is fully up.
static void* s_backlight_ctx;
static uint8_t s_brightness_percent;

void ui_log_memory(const char* stage)
{
    ESP_LOGI(
        TAG,
        "%s: internal free=%u min=%u largest=%u, DMA free=%u largest=%u",
        stage,
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
        heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
        heap_caps_get_free_size(MALLOC_CAP_DMA),
        heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
}

static bool flush_pending(void)
{
    return s_disp->driver->draw_buf->flushing != 0;
}

// lv_refr_now() returns while the last SPI transfer may still be in flight;
// the port clears the flag from the transfer-done callback.
static esp_err_t wait_first_flush(void)
{
    const int64_t deadline = esp_timer_get_time() + FIRST_FLUSH_TIMEOUT_US;
    while (esp_timer_get_time() < deadline) {
        ESP_RETURN_ON_FALSE(lvgl_port_lock(LOCK_TIMEOUT_MS), ESP_ERR_TIMEOUT, TAG, "LVGL lock");
        const bool pending = flush_pending();
        lvgl_port_unlock();

        if (!pending) {
            return ESP_OK;
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }

    return ESP_ERR_TIMEOUT;
}

esp_err_t ui_init(const ui_config_t* config)
{
    ESP_RETURN_ON_FALSE(
        config != NULL && config->panel_io != NULL && config->panel != NULL && config->set_backlight != NULL &&
            config->brightness_percent <= 100,
        ESP_ERR_INVALID_ARG,
        TAG,
        "config");
    ESP_RETURN_ON_FALSE(s_disp == NULL, ESP_ERR_INVALID_STATE, TAG, "already initialized");

    // The display holds light-sleep off while it is ON or DIM.
    ESP_RETURN_ON_ERROR(esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "display", &s_pm_lock), TAG, "PM lock");
    ESP_RETURN_ON_ERROR(esp_pm_lock_acquire(s_pm_lock), TAG, "PM lock acquire");

    ui_log_memory("before LVGL");
    // The SPI bus interrupt lives on the core that initialized the bus, the one
    // calling here. Flushing from the other core hung the LVGL task for good in
    // spi_device_acquire_bus (ESP-IDF 5.5.3), so the task stays on this core.
    lvgl_port_cfg_t port_config = ESP_LVGL_PORT_INIT_CONFIG();
    port_config.task_affinity = xPortGetCoreID();
    ESP_RETURN_ON_ERROR(lvgl_port_init(&port_config), TAG, "LVGL port");

    const lvgl_port_display_cfg_t display_config = {
        .io_handle = config->panel_io,
        .panel_handle = config->panel,
        .buffer_size = config->width * UI_DRAW_BUFFER_LINES,
        .double_buffer = true,
        .hres = config->width,
        .vres = config->height,
        .rotation =
            {
                .swap_xy = config->swap_xy,
                .mirror_x = config->mirror_x,
                .mirror_y = config->mirror_y,
            },
        .flags =
            {
                .buff_dma = true,
            },
    };
    s_disp = lvgl_port_add_disp(&display_config);
    ESP_RETURN_ON_FALSE(s_disp != NULL, ESP_ERR_NO_MEM, TAG, "LVGL display");
    ui_log_memory("draw buffers");

    const ui_model_t start = {.screen = UI_SCREEN_START};
    ESP_RETURN_ON_FALSE(lvgl_port_lock(LOCK_TIMEOUT_MS), ESP_ERR_TIMEOUT, TAG, "LVGL lock");
    ui_fonts_load(&s_fonts);
    ui_view_create(lv_disp_get_scr_act(s_disp), &s_fonts);
    ui_view_apply(&start);
    lv_refr_now(s_disp);
    lvgl_port_unlock();
    ui_log_memory("screens");

    const int64_t flush_started = esp_timer_get_time();
    ESP_RETURN_ON_ERROR(wait_first_flush(), TAG, "first flush");

    // Panel RAM keeps the last frame through a reset: after deep sleep the shutdown
    // question was visible before the new frame. Display output starts only once
    // the new frame is in RAM.
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(config->panel, true), TAG, "display on");
    ESP_RETURN_ON_ERROR(config->set_backlight(config->backlight_ctx, config->brightness_percent), TAG, "backlight");
    s_backlight_ctx = config->backlight_ctx;
    s_brightness_percent = config->brightness_percent;
    s_set_backlight = config->set_backlight;

    ESP_LOGI(
        TAG,
        "first frame flushed (waited %lld us), backlight %u%%",
        esp_timer_get_time() - flush_started,
        config->brightness_percent);

    return ESP_OK;
}

esp_err_t ui_apply_model(const ui_model_t* model)
{
    ESP_RETURN_ON_FALSE(model != NULL, ESP_ERR_INVALID_ARG, TAG, "model");
    if (s_set_backlight == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // A busy render is not an error: the caller keeps the model and retries.
    if (!lvgl_port_lock(APPLY_LOCK_TIMEOUT_MS)) {
        return ESP_ERR_TIMEOUT;
    }

    ui_view_apply(model);
    lvgl_port_unlock();

    return ESP_OK;
}

esp_err_t ui_set_display_state(ui_display_state_t state, uint8_t brightness_percent)
{
    ESP_RETURN_ON_FALSE(brightness_percent <= 100, ESP_ERR_INVALID_ARG, TAG, "brightness");
    if (s_set_backlight == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // OFF has to stop the LVGL task before the panel sleeps; the pinned port cannot.
    if (state == UI_DISPLAY_OFF) {
        return ESP_ERR_NOT_SUPPORTED;
    }

    if (brightness_percent == s_brightness_percent) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(s_set_backlight(s_backlight_ctx, brightness_percent), TAG, "backlight");
    s_brightness_percent = brightness_percent;

    return ESP_OK;
}
