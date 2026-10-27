#include "ui_render.h"

#include <stdatomic.h>

#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "ui_fonts.h"
#include "ui_view.h"

#define LOCK_TIMEOUT_MS 1000
#define APPLY_LOCK_TIMEOUT_MS 100
#define FRAME_TIMEOUT_MS 1000
#define LAST_FLUSH_TIMEOUT_US (100 * 1000) // One draw buffer takes about 1 ms at 40 MHz.

_Static_assert(ATOMIC_BOOL_LOCK_FREE == 2, "SPI completion tracking requires lock-free atomics");

static const char* TAG = "ui_render";

static lv_display_t* s_disp;
static esp_lcd_panel_io_handle_t s_panel_io;
static bool s_port_ready;
static ui_fonts_t s_fonts;
static SemaphoreHandle_t s_frame_done; // Given by the rendering task after a requested refresh.
static bool s_frame_requested;         // Under the renderer lock.
static bool s_quiesced;                // Lock held by s_quiesce_owner; tick and timers stopped.
static TaskHandle_t s_quiesce_owner;
static atomic_bool s_flushing;

bool ui_render_owned_by_caller(void)
{
    return !s_quiesced || s_quiesce_owner == xTaskGetCurrentTaskHandle();
}

static void log_memory(const char* stage)
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

// LVGL emits this before calling the port's flush callback, under its lock.
static void on_flush_start(lv_event_t* event)
{
    atomic_store_explicit(&s_flushing, true, memory_order_release);
}

// Extends the port's completion callback with our own transfer state. SPI calls
// this from its ISR only after the last chunk of a color transfer has completed.
static bool on_color_trans_done(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t* event, void* context)
{
    lv_display_flush_ready(context);
    atomic_store_explicit(&s_flushing, false, memory_order_release);

    return false;
}

// A refresh can end before its last SPI transfer. The completion ISR clears our
// flag without taking the LVGL lock, including while the renderer is quiesced.
static esp_err_t wait_flush_done(int64_t timeout_us)
{
    const int64_t deadline = esp_timer_get_time() + timeout_us;
    while (esp_timer_get_time() < deadline) {
        ESP_RETURN_ON_FALSE(lvgl_port_lock(LOCK_TIMEOUT_MS), ESP_ERR_TIMEOUT, TAG, "LVGL lock");
        const bool pending = atomic_load_explicit(&s_flushing, memory_order_acquire);
        lvgl_port_unlock();

        if (!pending) {
            return ESP_OK;
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }

    return ESP_ERR_TIMEOUT;
}

// Runs in the LVGL task, under its lock, at the end of every refresh.
static void on_refresh_ready(lv_event_t* event)
{
    if (s_frame_requested) {
        s_frame_requested = false;
        xSemaphoreGive(s_frame_done);
    }
}

// Has the LVGL task draw the whole screen and waits until its last transfer is
// done. Only the LVGL task renders: its stack is sized for LVGL 9 rendering, which
// overflowed the 4 KiB stack of the main task.
esp_err_t ui_render_draw_frame(void)
{
    ESP_RETURN_ON_FALSE(ui_render_owned_by_caller(), ESP_ERR_INVALID_STATE, TAG, "renderer stopped by another task");

    if (s_quiesced) {
        ESP_RETURN_ON_ERROR(lvgl_port_resume(), TAG, "LVGL resume");
        s_quiesced = false; // Its lock is released below.
        s_quiesce_owner = NULL;
    } else {
        ESP_RETURN_ON_FALSE(lvgl_port_lock(LOCK_TIMEOUT_MS), ESP_ERR_TIMEOUT, TAG, "LVGL lock");
    }

    // No refresh can run while the lock is held, so a pending signal is stale.
    xSemaphoreTake(s_frame_done, 0);
    s_frame_requested = true;
    lv_obj_invalidate(lv_display_get_screen_active(s_disp));
    lvgl_port_unlock();

    ESP_RETURN_ON_FALSE(
        xSemaphoreTake(s_frame_done, pdMS_TO_TICKS(FRAME_TIMEOUT_MS)) == pdTRUE, ESP_ERR_TIMEOUT, TAG, "refresh");

    return wait_flush_done(LAST_FLUSH_TIMEOUT_US);
}

esp_err_t ui_render_init(const ui_config_t* config)
{
    ESP_RETURN_ON_FALSE(s_disp == NULL, ESP_ERR_INVALID_STATE, TAG, "already initialized");

    s_frame_done = xSemaphoreCreateBinary();
    ESP_RETURN_ON_FALSE(s_frame_done != NULL, ESP_ERR_NO_MEM, TAG, "frame semaphore");

    log_memory("before LVGL");
    // The SPI bus interrupt lives on the core that initialized the bus, the one
    // calling here. Flushing from the other core hung the LVGL task for good in
    // spi_device_acquire_bus (ESP-IDF 5.5.3), so the task stays on this core.
    lvgl_port_cfg_t port_config = ESP_LVGL_PORT_INIT_CONFIG();
    port_config.task_affinity = xPortGetCoreID();
    ESP_RETURN_ON_ERROR(lvgl_port_init(&port_config), TAG, "LVGL port");
    s_port_ready = true;
    s_panel_io = config->panel_io;

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
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags =
            {
                .buff_dma = true,
                .swap_bytes = true, // The ST7789 takes RGB565 big-endian over SPI.
            },
    };
    // Keep the task out until both callbacks are attached. add_disp() takes the
    // same recursive lock internally, so no untracked transfer can start here.
    ESP_RETURN_ON_FALSE(lvgl_port_lock(LOCK_TIMEOUT_MS), ESP_ERR_TIMEOUT, TAG, "LVGL lock");
    s_disp = lvgl_port_add_disp(&display_config);
    if (s_disp == NULL) {
        lvgl_port_unlock();

        return ESP_ERR_NO_MEM;
    }

    const esp_lcd_panel_io_callbacks_t callbacks = {.on_color_trans_done = on_color_trans_done};
    esp_err_t err = esp_lcd_panel_io_register_event_callbacks(s_panel_io, &callbacks, s_disp);
    if (err != ESP_OK) {
        // Completion is no longer observable by us: retain resources on cleanup.
        atomic_store_explicit(&s_flushing, true, memory_order_release);
        lvgl_port_unlock();

        return err;
    }

    lv_display_add_event_cb(s_disp, on_flush_start, LV_EVENT_FLUSH_START, NULL);
    log_memory("draw buffers");

    const ui_model_t start = {.screen = UI_SCREEN_START};
    lv_display_add_event_cb(s_disp, on_refresh_ready, LV_EVENT_REFR_READY, NULL);
    ui_fonts_load(&s_fonts);
    err = ui_view_create(lv_display_get_screen_active(s_disp), &s_fonts) == LV_RESULT_OK ? ESP_OK : ESP_ERR_NO_MEM;
    if (err == ESP_OK) {
        ui_view_apply(&start);
    }

    lvgl_port_unlock();
    ESP_RETURN_ON_ERROR(err, TAG, "view");
    log_memory("screens");

    return ESP_OK;
}

esp_err_t ui_render_abort_init(void)
{
    if (s_port_ready && s_disp != NULL) {
        ESP_RETURN_ON_ERROR(ui_render_quiesce(), TAG, "stop before cleanup");

        // The port's SPI completion callback borrows the display pointer. No
        // transfer remains and the render lock prevents a new one from starting.
        const esp_lcd_panel_io_callbacks_t callbacks = {0};
        ESP_RETURN_ON_ERROR(
            esp_lcd_panel_io_register_event_callbacks(s_panel_io, &callbacks, NULL), TAG, "detach callback");

        s_frame_requested = false;
        ui_view_deinit();
        ESP_RETURN_ON_ERROR(lvgl_port_remove_disp(s_disp), TAG, "remove display");
        s_disp = NULL;
        ui_fonts_unload(&s_fonts);

        s_quiesced = false;
        s_quiesce_owner = NULL;
        lvgl_port_unlock();
    }

    if (s_frame_done != NULL) {
        vSemaphoreDelete(s_frame_done);
        s_frame_done = NULL;
    }

    if (s_port_ready) {
        // The port task releases its own timer, mutexes and runtime after this
        // request. Its API has no completion signal, so this boot cannot re-init.
        ESP_RETURN_ON_ERROR(lvgl_port_deinit(), TAG, "port shutdown");
        s_port_ready = false;
    }

    s_panel_io = NULL;

    return ESP_OK;
}

esp_err_t ui_render_apply_model(const ui_model_t* model)
{
    ESP_RETURN_ON_FALSE(ui_render_owned_by_caller(), ESP_ERR_INVALID_STATE, TAG, "renderer stopped by another task");

    // A busy render is not an error: the caller keeps the model and retries.
    if (!lvgl_port_lock(APPLY_LOCK_TIMEOUT_MS)) {
        return ESP_ERR_TIMEOUT;
    }

    ui_view_apply(model);
    lvgl_port_unlock();

    return ESP_OK;
}

// Stops rendering for as long as the display is OFF. lvgl_port_stop() alone leaves
// the LVGL task polling every two ticks, which keeps the chip out of light sleep;
// with the lock kept by this task, the LVGL task blocks on it until the display wakes.
// Holding the lock also proves that no render is under way.
esp_err_t ui_render_quiesce(void)
{
    ESP_RETURN_ON_FALSE(ui_render_owned_by_caller(), ESP_ERR_INVALID_STATE, TAG, "renderer stopped by another task");

    if (s_quiesced) {
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

    s_quiesced = true;
    s_quiesce_owner = xTaskGetCurrentTaskHandle();

    return ESP_OK;
}
