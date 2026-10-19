#include <string.h>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sensing_internal.h"

#define I2C_SPEED_HZ 100000
#define PROBE_TIMEOUT_MS 50
#define TRANSFER_TIMEOUT_MS 100
#define WRITE_MAX 32 // bme68x writes at most 2 * BME68X_LEN_INTERLEAVE_BUFF - 1 bytes.

static const char* TAG = "sensing_bme68x";

static const uint8_t ADDRESSES[] = {BME68X_I2C_ADDR_HIGH, BME68X_I2C_ADDR_LOW};

static BME68X_INTF_RET_TYPE bus_read(uint8_t reg, uint8_t* data, uint32_t length, void* intf_ptr)
{
    i2c_master_dev_handle_t device = intf_ptr;
    const esp_err_t err = i2c_master_transmit_receive(device, &reg, 1, data, length, TRANSFER_TIMEOUT_MS);

    return err == ESP_OK ? BME68X_INTF_RET_SUCCESS : -1;
}

static BME68X_INTF_RET_TYPE bus_write(uint8_t reg, const uint8_t* data, uint32_t length, void* intf_ptr)
{
    if (length > WRITE_MAX) {
        return -1;
    }

    uint8_t buffer[1 + WRITE_MAX];
    buffer[0] = reg;
    memcpy(&buffer[1], data, length);

    i2c_master_dev_handle_t device = intf_ptr;
    const esp_err_t err = i2c_master_transmit(device, buffer, length + 1, TRANSFER_TIMEOUT_MS);

    return err == ESP_OK ? BME68X_INTF_RET_SUCCESS : -1;
}

// Sleeps for whole ticks and busy-waits only for what is shorter than a tick.
static void delay_us(uint32_t period_us, void* intf_ptr)
{
    const uint32_t tick_us = portTICK_PERIOD_MS * 1000;
    if (period_us >= tick_us) {
        vTaskDelay((period_us + tick_us - 1) / tick_us);

        return;
    }

    esp_rom_delay_us(period_us);
}

static esp_err_t add_device(i2c_master_bus_handle_t bus, uint8_t address, sensing_sensor_t* sensor)
{
    const i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = I2C_SPEED_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &config, &sensor->device), TAG, "add device");

    sensor->address = address;
    sensor->bme = (struct bme68x_dev){
        .intf = BME68X_I2C_INTF,
        .intf_ptr = sensor->device,
        .read = bus_read,
        .write = bus_write,
        .delay_us = delay_us,
        .amb_temp = 25,
    };

    return ESP_OK;
}

esp_err_t sensing_sensor_open(i2c_master_bus_handle_t bus, sensing_sensor_t* sensor)
{
    for (size_t i = 0; i < sizeof(ADDRESSES); i++) {
        if (i2c_master_probe(bus, ADDRESSES[i], PROBE_TIMEOUT_MS) != ESP_OK) {
            continue;
        }

        ESP_RETURN_ON_ERROR(add_device(bus, ADDRESSES[i], sensor), TAG, "device");
        const int8_t rc = bme68x_init(&sensor->bme);
        if (rc != BME68X_OK) {
            ESP_LOGE(TAG, "bme68x_init at 0x%02x: %d", ADDRESSES[i], rc);
            sensing_sensor_close(sensor);

            return ESP_FAIL;
        }

        ESP_LOGI(
            TAG, "BME68x chip 0x%02x variant %lu at 0x%02x", sensor->bme.chip_id, sensor->bme.variant_id, ADDRESSES[i]);

        return ESP_OK;
    }

    return ESP_ERR_NOT_FOUND;
}

void sensing_sensor_close(sensing_sensor_t* sensor)
{
    if (sensor->device == NULL) {
        return;
    }

    i2c_master_bus_rm_device(sensor->device);
    sensor->device = NULL;
}

esp_err_t sensing_sensor_measure(
    sensing_sensor_t* sensor,
    const bsec_bme_settings_t* settings,
    struct bme68x_data* data,
    bool* has_data)
{
    struct bme68x_dev* bme = &sensor->bme;
    *has_data = false;

    struct bme68x_conf conf;
    ESP_RETURN_ON_FALSE(bme68x_get_conf(&conf, bme) == BME68X_OK, ESP_FAIL, TAG, "get conf");
    conf.os_hum = settings->humidity_oversampling;
    conf.os_temp = settings->temperature_oversampling;
    conf.os_pres = settings->pressure_oversampling;
    conf.filter = BME68X_FILTER_OFF;
    conf.odr = BME68X_ODR_NONE;
    ESP_RETURN_ON_FALSE(bme68x_set_conf(&conf, bme) == BME68X_OK, ESP_FAIL, TAG, "set conf");

    const struct bme68x_heatr_conf heater = {
        .enable = BME68X_ENABLE,
        .heatr_temp = settings->heater_temperature,
        .heatr_dur = settings->heater_duration,
    };
    ESP_RETURN_ON_FALSE(
        bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heater, bme) == BME68X_OK, ESP_FAIL, TAG, "heater conf");
    ESP_RETURN_ON_FALSE(bme68x_set_op_mode(BME68X_FORCED_MODE, bme) == BME68X_OK, ESP_FAIL, TAG, "forced mode");

    // The heater runs after the TPH conversion; wait for both before reading.
    const uint32_t wait_us = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, bme) + heater.heatr_dur * 1000;
    delay_us(wait_us, NULL);

    uint8_t count = 0;
    const int8_t rc = bme68x_get_data(BME68X_FORCED_MODE, data, &count, bme);
    if (rc == BME68X_W_NO_NEW_DATA) {
        return ESP_OK;
    }

    ESP_RETURN_ON_FALSE(rc == BME68X_OK, ESP_FAIL, TAG, "get data: %d", rc);
    *has_data = count > 0;

    return ESP_OK;
}
