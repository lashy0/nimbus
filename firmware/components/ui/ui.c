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
#define LAST_FLUSH_TIMEOUT_US (100 * 1000) // One draw buffer takes about 1 ms at 40 MHz.
#define PANEL_SLEEP_GAP_US (120 * 1000)    // ST7789: between Sleep In and Sleep Out either way.

static const char* TAG = "ui";

static const char* const STATE_NAMES[] = {
    [UI_DISPLAY_ON] = "ON",
    [UI_DISPLAY_DIM] = "DIM",
    [UI_DISPLAY_OFF] = "OFF",
};

static esp_pm_lock_handle_t s_pm_lock;
static lv_disp_t* s_disp;
static esp_lcd_panel_handle_t s_panel;
static ui_fonts_t s_fonts;
static ui_backlight_fn s_set_backlight; // Set once the display is fully up.
static ui_backlight_pin_fn s_release_backlight;
static ui_backlight_pin_fn s_reclaim_backlight;
static void* s_backlight_ctx;

// What the display has reached. A failed transition keeps the flags of the steps
// already done, and the next request continues from them.
typedef struct {
    ui_display_state_t state;
    bool settled; // state was reached in full.
    bool pm_held;
    bool quiesced; // LVGL lock held by quiesce_owner; tick and timers stopped.
    TaskHandle_t quiesce_owner;
    bool panel_asleep;
    bool backlight_released;
    int64_t panel_sleep_changed_us;
    uint8_t brightness_percent;
} display_t;

static display_t s_display;

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

// lv_refr_now() returns while the last SPI transfer may still be in flight; the
// port clears the flag from the transfer-done interrupt, which takes no lock, so
// the wait also works while this task keeps the display quiesced.
static esp_err_t wait_flush_done(int64_t timeout_us)
{
    const int64_t deadline = esp_timer_get_time() + timeout_us;
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
            config->release_backlight != NULL && config->reclaim_backlight != NULL && config->brightness_percent <= 100,
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
    ESP_RETURN_ON_ERROR(wait_flush_done(FIRST_FLUSH_TIMEOUT_US), TAG, "first flush");

    // Panel RAM keeps the last frame through a reset: after deep sleep the shutdown
    // question was visible before the new frame. Display output starts only once
    // the new frame is in RAM.
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(config->panel, true), TAG, "display on");
    ESP_RETURN_ON_ERROR(config->set_backlight(config->backlight_ctx, config->brightness_percent), TAG, "backlight");
    s_panel = config->panel;
    s_backlight_ctx = config->backlight_ctx;
    s_release_backlight = config->release_backlight;
    s_reclaim_backlight = config->reclaim_backlight;
    s_set_backlight = config->set_backlight;

    // The panel left Sleep In during its init, shortly before.
    s_display = (display_t){
        .state = UI_DISPLAY_ON,
        .settled = true,
        .pm_held = true,
        .panel_sleep_changed_us = esp_timer_get_time(),
        .brightness_percent = config->brightness_percent,
    };

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

// Stops rendering for as long as the display is OFF. lvgl_port_stop() alone leaves
// the LVGL task polling every 5 ms, which keeps the chip out of light sleep; with
// the lock kept by this task, the LVGL task blocks on it until the display wakes.
// Holding the lock also proves that no render is under way.
static esp_err_t quiesce(void)
{
    if (s_display.quiesced) {
        return ESP_OK;
    }

    if (!lvgl_port_lock(APPLY_LOCK_TIMEOUT_MS)) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = lvgl_port_stop();
    if (err == ESP_OK) {
        err = wait_flush_done(LAST_FLUSH_TIMEOUT_US);
    }

    if (err != ESP_OK) {
        lvgl_port_resume();
        lvgl_port_unlock();

        return err;
    }

    s_display.quiesced = true;
    s_display.quiesce_owner = xTaskGetCurrentTaskHandle();

    return ESP_OK;
}

// Draws the whole screen again before the panel shows it.
static esp_err_t redraw(void)
{
    if (s_display.quiesced) {
        ESP_RETURN_ON_ERROR(lvgl_port_resume(), TAG, "LVGL resume");
        s_display.quiesced = false; // Its lock is released below.
    } else {
        ESP_RETURN_ON_FALSE(lvgl_port_lock(LOCK_TIMEOUT_MS), ESP_ERR_TIMEOUT, TAG, "LVGL lock");
    }

    lv_obj_invalidate(lv_disp_get_scr_act(s_disp));
    lv_refr_now(s_disp);
    lvgl_port_unlock();

    return wait_flush_done(FIRST_FLUSH_TIMEOUT_US);
}

static esp_err_t turn_off(void)
{
    // Nothing has changed yet if LVGL is busy.
    ESP_RETURN_ON_ERROR(quiesce(), TAG, "stop LVGL");
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
    ESP_RETURN_ON_ERROR(redraw(), TAG, "first frame");
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

esp_err_t ui_set_display_state(ui_display_state_t state, uint8_t brightness_percent)
{
    ESP_RETURN_ON_FALSE(state <= UI_DISPLAY_OFF && brightness_percent <= 100, ESP_ERR_INVALID_ARG, TAG, "arguments");
    if (s_set_backlight == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // A recursive mutex can only be released by the task that holds it.
    ESP_RETURN_ON_FALSE(
        !s_display.quiesced || s_display.quiesce_owner == xTaskGetCurrentTaskHandle(),
        ESP_ERR_INVALID_STATE,
        TAG,
        "display is OFF for another task");

    if (state == UI_DISPLAY_OFF) {
        return s_display.settled && s_display.state == UI_DISPLAY_OFF ? ESP_OK : turn_off();
    }

    return turn_on(state, brightness_percent);
}
