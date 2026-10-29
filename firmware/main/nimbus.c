#include <sys/param.h>

#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "esp_app_desc.h"
#include "esp_check.h"
#include "esp_idf_version.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "display.h"
#include "sensing.h"

#define PIN_I2C_SDA GPIO_NUM_21
#define PIN_I2C_SCL GPIO_NUM_22

#define LCD_SPI_HOST SPI2_HOST
#define PIN_LCD_MOSI GPIO_NUM_19
#define PIN_LCD_SCLK GPIO_NUM_18
#define PIN_LCD_CS GPIO_NUM_5
#define PIN_LCD_DC GPIO_NUM_16
#define PIN_LCD_RESET GPIO_NUM_23
#define PIN_LCD_BACKLIGHT GPIO_NUM_4

#define SENSING_CORE 0

#define LOG_PERIOD_MS 3000

#define TEST_STRIP_LINES 24
#define TEST_FRAME_PX 2
// The panel reads RGB565 high byte first.
#define RGB565_BE(color) ((uint16_t)(((color) >> 8) | ((color) << 8)))

static const char* TAG = "nimbus";

// Constant once filled, so queued transfers may share them.
static uint16_t s_test_fill[DISPLAY_WIDTH * TEST_STRIP_LINES];
static uint16_t s_test_frame[DISPLAY_HEIGHT * TEST_FRAME_PX];

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

static esp_err_t create_spi_bus(void)
{
    const spi_bus_config_t config = {
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = -1, // The panel is write-only here.
        .sclk_io_num = PIN_LCD_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t),
    };

    return spi_bus_initialize(LCD_SPI_HOST, &config, SPI_DMA_CH_AUTO);
}

// Temporary check of colors, inversion and gap: red screen, white frame at the edges.
static esp_err_t draw_test_frame(esp_lcd_panel_handle_t panel)
{
    for (size_t i = 0; i < sizeof(s_test_fill) / sizeof(s_test_fill[0]); i++) {
        s_test_fill[i] = RGB565_BE(0xF800);
    }
    for (size_t i = 0; i < sizeof(s_test_frame) / sizeof(s_test_frame[0]); i++) {
        s_test_frame[i] = RGB565_BE(0xFFFF);
    }

    for (int y = 0; y < DISPLAY_HEIGHT; y += TEST_STRIP_LINES) {
        const int y_end = MIN(y + TEST_STRIP_LINES, DISPLAY_HEIGHT);
        ESP_RETURN_ON_ERROR(esp_lcd_panel_draw_bitmap(panel, 0, y, DISPLAY_WIDTH, y_end, s_test_fill), TAG, "fill");
    }

    const int w = DISPLAY_WIDTH;
    const int h = DISPLAY_HEIGHT;
    const int px = TEST_FRAME_PX;
    ESP_RETURN_ON_ERROR(esp_lcd_panel_draw_bitmap(panel, 0, 0, w, px, s_test_frame), TAG, "top");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_draw_bitmap(panel, 0, h - px, w, h, s_test_frame), TAG, "bottom");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_draw_bitmap(panel, 0, 0, px, h, s_test_frame), TAG, "left");

    return esp_lcd_panel_draw_bitmap(panel, w - px, 0, w, h, s_test_frame);
}

static esp_err_t start_display(void)
{
    ESP_RETURN_ON_ERROR(create_spi_bus(), TAG, "SPI bus");

    const display_config_t config = {
        .spi_host = LCD_SPI_HOST,
        .cs = PIN_LCD_CS,
        .dc = PIN_LCD_DC,
        .reset = PIN_LCD_RESET,
        .backlight = PIN_LCD_BACKLIGHT,
    };
    display_t display;
    ESP_RETURN_ON_ERROR(display_open(&config, &display), TAG, "open");
    ESP_RETURN_ON_ERROR(draw_test_frame(display.panel), TAG, "test frame");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(display.panel, true), TAG, "output on");

    return display_set_brightness(100);
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

    // Not fatal: sensing still runs and logs.
    err = start_display();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "display: %s", esp_err_to_name(err));
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
