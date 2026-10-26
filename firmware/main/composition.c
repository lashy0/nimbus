#include "composition.h"

#include "app.h"
#include "board.h"
#include "board_backlight.h"
#include "board_battery.h"
#include "board_buttons.h"
#include "board_display.h"
#include "board_i2c.h"
#include "board_sleep.h"
#include "esp_attr.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_sleep.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "power.h"
#include "restart_guard.h"
#include "sdkconfig.h"
#include "sensing.h"
#include "storage.h"
#include "ui.h"

#define ASSETS_PARTITION "storage"
#define ASSETS_MOUNT_POINT "/spiffs"
#define STABLE_RUN_US (5LL * 60 * 1000 * 1000) // Without a crash this long, the crash series ends.

static const char* TAG = "composition";

// Survives panic, watchdog and deep sleep; checked before it is trusted.
static RTC_NOINIT_ATTR restart_record_t s_restart_record;
static bool s_power_ready;

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
        .timestamp_ms = event->timestamp_us / 1000,
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
        .release_backlight = board_backlight_release,
        .reclaim_backlight = board_backlight_reclaim,
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
    s_power_ready = true;

    if (!power_check_startup(esp_reset_reason() == ESP_RST_BROWNOUT)) {
        power_enter_sleep(POWER_SLEEP_STARTUP);
    }

    return ESP_OK;
}

static restart_cause_t restart_cause(void)
{
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:
            return RESTART_POWER_ON;
        case ESP_RST_PANIC:
        case ESP_RST_TASK_WDT:
        case ESP_RST_INT_WDT:
        case ESP_RST_WDT:
        case ESP_RST_BROWNOUT:
            return RESTART_CRASH;
        default:
            return RESTART_OTHER;
    }
}

// Firmware that crashes or browns out on every boot would restart forever, flashing the screen
// and draining the battery; after a few crashes in a row it sleeps instead.
static void check_crash_loop(void)
{
    const restart_cause_t cause = restart_cause();
    if (restart_guard_on_boot(&s_restart_record, cause)) {
        ESP_LOGE(TAG, "%lu crashes in a row; sleeping until a button press", s_restart_record.crashes);
        power_enter_sleep(POWER_SLEEP_FAULT);
    }

    if (cause == RESTART_CRASH) {
        ESP_LOGW(TAG, "crash %lu in a row; the boot stops at %d", s_restart_record.crashes, RESTART_CRASH_LIMIT);
    }
}

static void end_crash_series(void* arg)
{
    if (s_restart_record.crashes > 0) {
        ESP_LOGI(TAG, "stable run; crash count cleared");
    }

    restart_guard_clear(&s_restart_record);
}

// Without the timer the count only clears at the next power-on.
static void start_stability_timer(void)
{
    const esp_timer_create_args_t args = {
        .callback = end_crash_series,
        .name = "stable_run",
    };
    esp_timer_handle_t timer;
    esp_err_t err = esp_timer_create(&args, &timer);
    if (err == ESP_OK) {
        err = esp_timer_start_once(timer, STABLE_RUN_US);
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "stability timer: %s", esp_err_to_name(err));
    }
}

// The chip light-sleeps whenever no lock forbids it: the display holds one while
// lit and the buttons while pressed. The CPU frequency stays fixed, since the
// backlight LEDC runs on APB, which a lower CPU frequency would slow down.
static void configure_power_management(void)
{
    const esp_pm_config_t config = {
        .max_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
        .min_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ,
        .light_sleep_enable = true,
    };

    // Without it the device works the same, only without light sleep.
    const esp_err_t err = esp_pm_configure(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "light sleep unavailable: %s", esp_err_to_name(err));
    }
}

esp_err_t composition_init(void)
{
    ESP_RETURN_ON_ERROR(board_init_safe_gpio(), TAG, "safe GPIO");
    configure_power_management();
    ESP_RETURN_ON_ERROR(start_power(), TAG, "power");
    check_crash_loop();

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

    // The screen is the only output the user sees in the case.
    const uint8_t brightness_percent = app_load_brightness();
    ESP_RETURN_ON_ERROR(start_display(brightness_percent), TAG, "display");

    // Buttons start first, so a failure of either leaves no app task running while
    // this task puts the device to sleep. Presses before app_start() are dropped.
    ESP_RETURN_ON_ERROR(board_buttons_init(forward_button, NULL), TAG, "buttons");
    const app_config_t app_config = {.brightness_percent = brightness_percent};
    ESP_RETURN_ON_ERROR(app_start(&app_config), TAG, "app");

    // Without the bus the sensor error page stays until the next boot; input still works.
    err = start_sensing();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "sensing unavailable: %s", esp_err_to_name(err));
    }

    start_stability_timer();

    return ESP_OK;
}

void composition_enter_fault_sleep(void)
{
    if (s_power_ready) {
        power_enter_sleep(POWER_SLEEP_FAULT);
    }

    board_enter_deep_sleep(NULL);
    esp_deep_sleep_start();
}
