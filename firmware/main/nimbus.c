#include "driver/i2c_master.h"
#include "esp_app_desc.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bme680.h"

#define PIN_I2C_SDA GPIO_NUM_21
#define PIN_I2C_SCL GPIO_NUM_22

#define MEASURE_PERIOD_MS 3000

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

void app_main(void)
{
    ESP_LOGI(TAG, "Nimbus %s, ESP-IDF %s", esp_app_get_description()->version, esp_get_idf_version());

    i2c_master_bus_handle_t bus;
    esp_err_t err = create_i2c_bus(&bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus: %s", esp_err_to_name(err));

        return;
    }

    bme680_handle_t sensor;
    err = bme680_open(bus, &sensor);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "BME680: %s", esp_err_to_name(err));

        return;
    }

    const bme680_config_t config = {
        .heater_temp_c = 320,
        .heater_ms = 150,
    };
    err = bme680_configure(sensor, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "BME680 configure: %s", esp_err_to_name(err));

        return;
    }

    for (;;) {
        bme680_sample_t sample;
        err = bme680_measure(sensor, &sample);
        if (err == ESP_OK) {
            ESP_LOGI(
                TAG,
                "T %.2f °C, RH %.2f %%, P %.0f Pa, gas %.0f Ohm%s",
                sample.temperature_c,
                sample.humidity_percent,
                sample.pressure_pa,
                sample.gas_ohm,
                sample.gas_valid ? "" : " (invalid)");
        } else {
            ESP_LOGW(TAG, "measure: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(MEASURE_PERIOD_MS));
    }
}
