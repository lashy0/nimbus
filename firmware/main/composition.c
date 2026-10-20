#include "composition.h"

#include "app.h"
#include "board.h"
#include "board_backlight.h"
#include "board_battery.h"
#include "board_buttons.h"
#include "board_display.h"
#include "board_i2c.h"
#include "board_sleep.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "power.h"
#include "sensing.h"
#include "storage.h"
#include "ui.h"

#define ASSETS_PARTITION "storage"
#define ASSETS_MOUNT_POINT "/spiffs"

static const char* TAG = "composition";

static void forward_button(void* ctx, const board_button_event_t* event)
{
    static const app_gesture_t GESTURES[] = {
        [BOARD_BUTTON_PRESSED] = APP_GESTURE_PRESSED,
        [BOARD_BUTTON_LONG] = APP_GESTURE_LONG,
        [BOARD_BUTTON_SHORT] = APP_GESTURE_SHORT,
        [BOARD_BUTTON_RELEASED] = APP_GESTURE_RELEASED,
    };

    const app_button_event_t forwarded = {
        .button = event->button == BOARD_BUTTON_PREV ? APP_BUTTON_PREV : APP_BUTTON_NEXT,
        .gesture = GESTURES[event->type],
    };
    app_post_button(&forwarded);
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

static esp_err_t start_display(uint8_t brightness_percent)
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
        .brightness_percent = brightness_percent,
    };

    return ui_init(&config);
}

static void forward_measurement(void* ctx)
{
    app_notify_measurement();
}

static esp_err_t start_sensing(void)
{
    i2c_master_bus_handle_t bus;
    ESP_RETURN_ON_ERROR(board_i2c_init(&bus), TAG, "I2C bus");

    const sensing_config_t config = {
        .bus = bus,
        .notify = forward_measurement,
    };

    return sensing_start(&config);
}

// Runs before the display and sensor add their load: a cell too low to start
// them goes back to sleep with the screen dark.
static esp_err_t start_power(void)
{
    // Without the ADC the voltage stays unknown: no percentage and no low-battery power-off.
    const esp_err_t err = board_battery_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "battery ADC unavailable: %s", esp_err_to_name(err));
    }

    const power_config_t config = {
        .read_battery = board_battery_read,
        .enter_sleep = board_enter_deep_sleep,
    };
    ESP_RETURN_ON_ERROR(power_init(&config), TAG, "init");

    if (!power_check_startup(esp_reset_reason() == ESP_RST_BROWNOUT)) {
        power_enter_sleep(POWER_SLEEP_STARTUP);
    }

    return ESP_OK;
}

esp_err_t composition_init(void)
{
    ESP_RETURN_ON_ERROR(board_init_safe_gpio(), TAG, "safe GPIO");
    ESP_RETURN_ON_ERROR(start_power(), TAG, "power");

    // Without NVS the device runs on defaults and BSEC calibrates after every boot.
    esp_err_t err = storage_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "storage unavailable: %s", esp_err_to_name(err));
    }

    // Missing assets leave the UI on built-in fonts and empty images.
    err = mount_assets();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "assets unavailable: %s", esp_err_to_name(err));
    }

    // The display is optional for the rest of the device: app and buttons keep working.
    const uint8_t brightness_percent = app_load_brightness();
    err = start_display(brightness_percent);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "display unavailable: %s", esp_err_to_name(err));
    }

    const app_config_t app_config = {.brightness_percent = brightness_percent};
    ESP_RETURN_ON_ERROR(app_start(&app_config), TAG, "app");
    ESP_RETURN_ON_ERROR(board_buttons_init(forward_button, NULL), TAG, "buttons");

    // Without the bus the screens show no values until the next boot; input still works.
    err = start_sensing();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "sensing unavailable: %s", esp_err_to_name(err));
    }

    return ESP_OK;
}
