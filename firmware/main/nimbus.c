#include "driver/i2c_master.h"
#include "esp_app_desc.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "sensing.h"

#define PIN_I2C_SDA GPIO_NUM_21
#define PIN_I2C_SCL GPIO_NUM_22

#define SENSING_CORE 0

#define LOG_PERIOD_MS 3000

static const char* TAG = "nimbus";

static esp_err_t create_i2c_bus(i2c_master_bus_handle_t* out)
{
    const i2c_master_bus_config_t config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        // The breakout has its own pull-ups; the internal ones only add margin.
        .flags.enable_internal_pullup = true,
    };

    return i2c_new_master_bus(&config, out);
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

    i2c_master_bus_handle_t bus;
    err = create_i2c_bus(&bus);
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
