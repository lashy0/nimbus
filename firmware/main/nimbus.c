#include "esp_app_desc.h"
#include "esp_check.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "board.h"
#include "buttons.h"
#include "display.h"
#include "power.h"
#include "sensing.h"
#include "ui.h"

#define SENSING_CORE 0
#define POWER_CORE 0
#define UI_CORE 1

#define LOG_PERIOD_MS 3000

// LVGL reads it through CONFIG_LV_FS_STDIO_PATH.
#define ASSETS_PARTITION "assets"
#define ASSETS_PATH "/assets"
#define ASSETS_MAX_FILES 4

static const char* TAG = "nimbus";

static esp_err_t start_power(void)
{
    const power_config_t config = {
        .battery = BOARD_PIN_BATTERY,
        .adc_enable = BOARD_PIN_ADC_ENABLE,
    };

    return power_start(&config, POWER_CORE);
}

static esp_err_t start_buttons(void)
{
    const buttons_config_t config = {
        .prev = BOARD_PIN_BUTTON_PREV,
        .next = BOARD_PIN_BUTTON_NEXT,
    };

    return buttons_start(&config);
}

static esp_err_t start_ui(void)
{
    ESP_RETURN_ON_ERROR(board_create_spi_bus(), TAG, "SPI bus");

    const display_config_t config = {
        .spi_host = BOARD_LCD_SPI_HOST,
        .cs = BOARD_PIN_LCD_CS,
        .dc = BOARD_PIN_LCD_DC,
        .reset = BOARD_PIN_LCD_RESET,
        .backlight = BOARD_PIN_LCD_BACKLIGHT,
    };
    display_t display;
    ESP_RETURN_ON_ERROR(display_open(&config, &display), TAG, "display");

    return ui_start(&display, UI_CORE);
}

static esp_err_t init_nvs(void)
{
    esp_err_t err = nvs_flash_init();

    // Full, or written by a newer NVS version: erase and start over.
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS: %s, erasing", esp_err_to_name(err));
        err = nvs_flash_erase();
        if (err == ESP_OK) {
            err = nvs_flash_init();
        }
    }

    return err;
}

static esp_err_t mount_assets(void)
{
    const esp_vfs_spiffs_conf_t config = {
        .base_path = ASSETS_PATH,
        .partition_label = ASSETS_PARTITION,
        .max_files = ASSETS_MAX_FILES,
        .format_if_mount_failed = false,
    };

    return esp_vfs_spiffs_register(&config);
}

static void log_snapshot(const sensing_snapshot_t* snapshot)
{
    switch (snapshot->status) {
        case SENSING_STATUS_NO_DATA:
            ESP_LOGI(TAG, "no data yet");
            break;
        case SENSING_STATUS_OK:
            ESP_LOGI(
                TAG,
                "T %.2f °C, RH %.2f %%, P %.1f hPa, IAQ %.0f (accuracy %u)",
                snapshot->temperature_c,
                snapshot->humidity_percent,
                snapshot->pressure_hpa,
                snapshot->iaq,
                snapshot->iaq_accuracy);
            break;
        case SENSING_STATUS_ERROR:
            ESP_LOGW(TAG, "sensing error");
            break;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Nimbus %s, ESP-IDF %s", esp_app_get_description()->version, esp_get_idf_version());

    // Not fatal: components run without saved data.
    esp_err_t err = init_nvs();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS: %s", esp_err_to_name(err));
    }

    // Not fatal: the UI runs without images.
    err = mount_assets();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "assets: %s", esp_err_to_name(err));
    }

    // Not fatal: the UI shows the battery as unknown.
    err = start_power();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "power: %s", esp_err_to_name(err));
    }

    // Not fatal: the pages just do not switch.
    err = start_buttons();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "buttons: %s", esp_err_to_name(err));
    }

    // Not fatal: sensing still runs and logs.
    err = start_ui();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "UI: %s", esp_err_to_name(err));
    }

    i2c_master_bus_handle_t bus;
    err = board_create_i2c_bus(&bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus: %s", esp_err_to_name(err));

        return;
    }

    err = sensing_start(bus, SENSING_CORE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "sensing: %s", esp_err_to_name(err));

        return;
    }

    for (;;) {
        sensing_snapshot_t snapshot;
        sensing_get_snapshot(&snapshot);
        log_snapshot(&snapshot);

        vTaskDelay(pdMS_TO_TICKS(LOG_PERIOD_MS));
    }
}
