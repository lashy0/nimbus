#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

#define BOARD_ADC_SAMPLE_COUNT 32

typedef struct {
    adc_oneshot_unit_handle_t unit;
    adc_cali_handle_t calibration;
} board_adc_t;

typedef struct {
    int raw_min;
    int raw_median;
    int raw_max;
    int input_mv_min;
    int input_mv_median;
    int input_mv_max;
    unsigned samples;
    unsigned clipped_samples;
    bool calibrated;
    int64_t duration_us;
} board_adc_reading_t;

// Call before ADC, display or PWM. Restores only the pins owned by this board.
esp_err_t board_init_safe_gpio(void);
esp_err_t board_adc_init(board_adc_t* adc);

// Diagnostic LOW/HIGH comparison; GPIO14 is returned LOW on every exit.
// mV refers to the ADC input, not a validated battery-voltage measurement.
esp_err_t board_adc_measure(board_adc_t* adc, bool enable, unsigned settle_us, board_adc_reading_t* out);

uint32_t board_buttons_pressed(void); // bit 0 PREV, bit 1 NEXT

// Terminal preparation: no SPI/LVGL calls. On success only deep-sleep may follow.
// Waits at most 3 s for release; held buttons are excluded from wake.
esp_err_t board_prepare_deep_sleep(bool next_only, uint32_t* wake_buttons);
