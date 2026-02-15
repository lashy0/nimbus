#pragma once

#include "driver/gpio.h"
#include "driver/ledc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Configuration structure for the backlight
 */
typedef struct {
    gpio_num_t gpio_num;              /*!< GPIO number used for backlight control */
    ledc_mode_t leds_mode;            /*!< LEDC speed mode (High/Low speed) */
    ledc_channel_t leds_channel;      /*!< LEDC channel (0-7) */
    ledc_timer_t leds_timer;          /*!< LEDC timer source (0-3) */
    ledc_timer_bit_t duty_resolution; /*!< Resolution of the PWM duty cycle */
    uint32_t freq_hz;                 /*!< PWM frequency in Hertz */
} backlight_config_t;

/**
 * @brief Handle structure for the backlight instance
 */
typedef struct {
    ledc_channel_t channel; /*!< Configured LEDC channel */
    ledc_mode_t leds_mode;  /*!< Configured LEDC speed mode */
    uint32_t duty;          /*!< Calculated max duty cycle value based on resolution */
} backlight_handle_t;

/**
 * @brief Initialize the backlight controller
 *
 * @param[in] config Pointer to the backlight configuration structure
 * @param[out] handle Pointer to the backlight handle structure
 * @return
 *         - ESP_ERR_INVALID_ARG if parameter is invalid
 *         - ESP_OK on success
 */
esp_err_t backlight_init(const backlight_config_t* config, backlight_handle_t* handle);

/**
 * @brief Set the backlight brightness
 *
 * @param[in] handle Pointer to the initialized backlight handle
 * @param[in] brightness_percent Brightness level in percent (0-100)
 * @return
 *         - ESP_ERR_INVALID_ARG if parameter is invalid
 *         - ESP_OK on success
 */
esp_err_t backlight_set_brightness(const backlight_handle_t* handle, uint8_t brightness_percent);

#ifdef __cplusplus
}
#endif