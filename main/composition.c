#include "composition.h"

#include "board.h"
#include "board_buttons.h"
#include "board_display.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "ui.h"

#define ASSETS_PARTITION "storage"
#define ASSETS_MOUNT_POINT "/spiffs"
#define DEFAULT_BRIGHTNESS_PERCENT 60

static const char* TAG = "composition";

// Until app exists, button events are only logged; nothing acts on them.
static void log_button_event(void* ctx, const board_button_event_t* event)
{
    static const char* const BUTTONS[BOARD_BUTTON_COUNT] = {"PREV", "NEXT"};
    static const char* const TYPES[] = {"PRESSED", "LONG", "SHORT", "RELEASED"};

    ESP_LOGI(TAG, "button %s %s t=%lld us", BUTTONS[event->button], TYPES[event->type], event->timestamp_us);
}

// Never formats: a damaged asset image must not be erased on boot.
static esp_err_t mount_assets(void)
{
    const esp_vfs_spiffs_conf_t config = {
        .base_path = ASSETS_MOUNT_POINT,
        .partition_label = ASSETS_PARTITION,
        .max_files = 4,
        .format_if_mount_failed = false,
    };
    ESP_RETURN_ON_ERROR(esp_vfs_spiffs_register(&config), TAG, "mount");

    size_t total = 0;
    size_t used = 0;
    ESP_RETURN_ON_ERROR(esp_spiffs_info(ASSETS_PARTITION, &total, &used), TAG, "info");
    ESP_LOGI(TAG, "assets mounted at %s: %u of %u bytes used", ASSETS_MOUNT_POINT, used, total);

    return ESP_OK;
}

static esp_err_t start_display(void)
{
    board_display_t display;
    const uint32_t max_transfer = BOARD_DISPLAY_WIDTH * UI_DRAW_BUFFER_LINES * sizeof(uint16_t);
    ESP_RETURN_ON_ERROR(board_display_init(max_transfer, &display), TAG, "panel");
    ESP_RETURN_ON_ERROR(board_backlight_init(), TAG, "backlight");

    const ui_config_t config = {
        .panel_io = display.panel_io,
        .panel = display.panel,
        .width = display.width,
        .height = display.height,
        .swap_xy = display.swap_xy,
        .mirror_x = display.mirror_x,
        .mirror_y = display.mirror_y,
        .set_backlight = board_backlight_set,
        .backlight_ctx = NULL,
        .brightness_percent = DEFAULT_BRIGHTNESS_PERCENT,
    };

    return ui_init(&config);
}

esp_err_t composition_init(void)
{
    ESP_RETURN_ON_ERROR(board_init_safe_gpio(), TAG, "safe GPIO");
    ESP_RETURN_ON_ERROR(board_buttons_init(log_button_event, NULL), TAG, "buttons");

    // Missing assets leave the UI on built-in fonts and warning symbols.
    esp_err_t err = mount_assets();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "assets unavailable: %s", esp_err_to_name(err));
    }

    // The display is optional for the rest of the device: buttons keep working.
    err = start_display();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "display unavailable: %s", esp_err_to_name(err));
    }

    return ESP_OK;
}
