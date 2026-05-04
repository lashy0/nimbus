#include "bme680_bus.h"

#include "bme680_internal.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "i2c_bus.h"

static const char* TAG = "bme680_bus";

BME68X_INTF_RET_TYPE bme680_bus_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t length, void* intf_ptr)
{
    i2c_bus_device_handle_t dev = (i2c_bus_device_handle_t)intf_ptr;
    if (!dev || !reg_data || length == 0U) {
        return BME68X_E_COM_FAIL;
    }

    esp_err_t ret = i2c_bus_read_bytes(dev, reg_addr, length, reg_data);
    if (ret != ESP_OK && s_ctx.initialized) {
        bme680_bus_recovery(s_ctx.sda_gpio, s_ctx.scl_gpio);
        ret = i2c_bus_read_bytes(dev, reg_addr, length, reg_data);
    }
    return (ret == ESP_OK) ? BME68X_INTF_RET_SUCCESS : BME68X_E_COM_FAIL;
}

BME68X_INTF_RET_TYPE bme680_bus_write(uint8_t reg_addr, const uint8_t* reg_data, uint32_t length, void* intf_ptr)
{
    i2c_bus_device_handle_t dev = (i2c_bus_device_handle_t)intf_ptr;
    if (!dev || !reg_data || length == 0U) {
        return BME68X_E_COM_FAIL;
    }

    esp_err_t ret = i2c_bus_write_bytes(dev, reg_addr, length, reg_data);
    return (ret == ESP_OK) ? BME68X_INTF_RET_SUCCESS : BME68X_E_COM_FAIL;
}

void bme680_bus_delay_us(uint32_t period, void* intf_ptr)
{
    (void)intf_ptr;
    esp_rom_delay_us(period);
}

void bme680_bus_recovery(gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    gpio_set_direction(sda_gpio, GPIO_MODE_INPUT);
    gpio_set_pull_mode(sda_gpio, GPIO_PULLUP_ONLY);
    gpio_set_direction(scl_gpio, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(scl_gpio, 1);

    int sda_state = gpio_get_level(sda_gpio);
    if (sda_state == 1) {
        return;
    }

    ESP_LOGW(TAG, "I2C SDA stuck low, attempting recovery");
    for (int i = 0; i < 9; i++) {
        gpio_set_level(scl_gpio, 0);
        esp_rom_delay_us(5);
        gpio_set_level(scl_gpio, 1);
        esp_rom_delay_us(5);
        if (gpio_get_level(sda_gpio) == 1) {
            break;
        }
    }

    /* Send STOP: SDA low->high while SCL high. */
    gpio_set_direction(sda_gpio, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(sda_gpio, 0);
    esp_rom_delay_us(5);
    gpio_set_level(scl_gpio, 1);
    esp_rom_delay_us(5);
    gpio_set_level(sda_gpio, 1);
    esp_rom_delay_us(5);

    gpio_set_direction(sda_gpio, GPIO_MODE_INPUT);
    gpio_set_pull_mode(sda_gpio, GPIO_PULLUP_ONLY);
    if (gpio_get_level(sda_gpio) == 1) {
        ESP_LOGI(TAG, "I2C bus recovery successful");
    } else {
        ESP_LOGE(TAG, "I2C bus recovery failed, SDA still low");
    }
}
