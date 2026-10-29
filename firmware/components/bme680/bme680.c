#include "bme680.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"

#include "bme68x.h"

#define I2C_SPEED_HZ 100000
#define PROBE_TIMEOUT_MS 50
#define TRANSFER_TIMEOUT_MS 100
#define AMBIENT_TEMP_C 25 // Initial guess for the heater calculation, before the first reading.

static const char* TAG = "bme680";

static const uint8_t ADDRESSES[] = {BME68X_I2C_ADDR_HIGH, BME68X_I2C_ADDR_LOW};

struct bme680 {
    i2c_master_dev_handle_t device;
    struct bme68x_dev bme;
    uint32_t wait_us; // Conversion and heater time; 0 until bme680_configure() succeeds.
};

static BME68X_INTF_RET_TYPE bus_read(uint8_t reg, uint8_t* data, uint32_t length, void* intf_ptr)
{
    i2c_master_dev_handle_t device = intf_ptr;
    const esp_err_t err = i2c_master_transmit_receive(device, &reg, 1, data, length, TRANSFER_TIMEOUT_MS);

    return err == ESP_OK ? BME68X_INTF_RET_SUCCESS : -1;
}

// bme68x_set_regs() passes the first register here and the rest of its
// interleaved buffer as data, so register and data fit in one such buffer.
static BME68X_INTF_RET_TYPE bus_write(uint8_t reg, const uint8_t* data, uint32_t length, void* intf_ptr)
{
    uint8_t buffer[BME68X_LEN_INTERLEAVE_BUFF];
    if (length >= sizeof(buffer)) {
        return -1;
    }

    buffer[0] = reg;
    memcpy(&buffer[1], data, length);

    i2c_master_dev_handle_t device = intf_ptr;
    const esp_err_t err = i2c_master_transmit(device, buffer, length + 1, TRANSFER_TIMEOUT_MS);

    return err == ESP_OK ? BME68X_INTF_RET_SUCCESS : -1;
}

// usleep() yields to other tasks for long waits and, unlike a bare vTaskDelay(),
// never returns early: the measurement wait must not end before the sensor does.
static void delay_us(uint32_t period_us, void* intf_ptr)
{
    usleep(period_us);
}

static esp_err_t init_at(struct bme680* dev, i2c_master_bus_handle_t bus, uint8_t address)
{
    const i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = I2C_SPEED_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &config, &dev->device), TAG, "add device");

    dev->bme = (struct bme68x_dev){
        .intf = BME68X_I2C_INTF,
        .intf_ptr = dev->device,
        .read = bus_read,
        .write = bus_write,
        .delay_us = delay_us,
        .amb_temp = AMBIENT_TEMP_C,
    };

    const int8_t rc = bme68x_init(&dev->bme);
    if (rc != BME68X_OK) {
        ESP_LOGE(TAG, "init at 0x%02x: %d", address, rc);
        i2c_master_bus_rm_device(dev->device);

        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "chip 0x%02x variant %lu at 0x%02x", dev->bme.chip_id, dev->bme.variant_id, address);

    return ESP_OK;
}

esp_err_t bme680_open(i2c_master_bus_handle_t bus, bme680_handle_t* out)
{
    for (size_t i = 0; i < sizeof(ADDRESSES); i++) {
        if (i2c_master_probe(bus, ADDRESSES[i], PROBE_TIMEOUT_MS) != ESP_OK) {
            continue;
        }

        struct bme680* dev = calloc(1, sizeof(*dev));
        if (dev == NULL) {
            return ESP_ERR_NO_MEM;
        }

        const esp_err_t err = init_at(dev, bus, ADDRESSES[i]);
        if (err != ESP_OK) {
            free(dev);

            return err;
        }

        *out = dev;

        return ESP_OK;
    }

    return ESP_ERR_NOT_FOUND;
}

esp_err_t bme680_configure(bme680_handle_t dev, const bme680_config_t* config)
{
    struct bme68x_dev* bme = &dev->bme;

    // A failed write may leave the sensor with only part of the new settings.
    dev->wait_us = 0;

    ESP_RETURN_ON_FALSE(
        config->temperature_os <= BME68X_OS_16X && config->pressure_os <= BME68X_OS_16X &&
            config->humidity_os <= BME68X_OS_16X,
        ESP_ERR_INVALID_ARG,
        TAG,
        "oversampling");

    // The IIR filter is off: at one reading every few seconds it would only
    // delay changes in temperature and pressure.
    struct bme68x_conf conf = {
        .os_temp = config->temperature_os,
        .os_pres = config->pressure_os,
        .os_hum = config->humidity_os,
        .filter = BME68X_FILTER_OFF,
        .odr = BME68X_ODR_NONE,
    };
    struct bme68x_heatr_conf heater = {
        .enable = config->gas_enabled ? BME68X_ENABLE : BME68X_DISABLE,
        .heatr_temp = config->heater_temp_c,
        .heatr_dur = config->heater_ms,
    };
    ESP_RETURN_ON_FALSE(bme68x_set_conf(&conf, bme) == BME68X_OK, ESP_FAIL, TAG, "conf");
    ESP_RETURN_ON_FALSE(bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heater, bme) == BME68X_OK, ESP_FAIL, TAG, "heater");

    // The heater runs after the temperature, pressure and humidity conversion.
    const uint32_t heater_us = config->gas_enabled ? (uint32_t)config->heater_ms * 1000 : 0;
    dev->wait_us = bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf, bme) + heater_us;

    return ESP_OK;
}

esp_err_t bme680_measure(bme680_handle_t dev, bme680_sample_t* out)
{
    struct bme68x_dev* bme = &dev->bme;
    if (dev->wait_us == 0) {
        return ESP_ERR_INVALID_STATE;
    }

    // Forced mode runs one measurement with the stored settings, then the sensor sleeps.
    ESP_RETURN_ON_FALSE(bme68x_set_op_mode(BME68X_FORCED_MODE, bme) == BME68X_OK, ESP_FAIL, TAG, "forced mode");
    delay_us(dev->wait_us, NULL);

    struct bme68x_data data;
    uint8_t count = 0;
    const int8_t rc = bme68x_get_data(BME68X_FORCED_MODE, &data, &count, bme);
    if (rc == BME68X_W_NO_NEW_DATA || (rc == BME68X_OK && count == 0)) {
        return ESP_ERR_TIMEOUT;
    }

    ESP_RETURN_ON_FALSE(rc == BME68X_OK, ESP_FAIL, TAG, "get data: %d", rc);

    *out = (bme680_sample_t){
        .temperature_c = data.temperature,
        .humidity_percent = data.humidity,
        .pressure_pa = data.pressure,
        .gas_ohm = data.gas_resistance,
        .gas_valid = (data.status & BME68X_GASM_VALID_MSK) != 0,
    };

    return ESP_OK;
}

void bme680_close(bme680_handle_t dev)
{
    if (dev == NULL) {
        return;
    }

    i2c_master_bus_rm_device(dev->device);
    free(dev);
}
