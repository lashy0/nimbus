#pragma once

#include <stdbool.h>
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BTN_ID_NONE = -1,
    BTN_ID_PREV = 0,
    BTN_ID_NEXT = 1
} button_id_t;

typedef void (*button_short_press_cb_t)(button_id_t btn_id);
typedef void (*button_long_press_cb_t)(button_id_t btn_id);

typedef struct {
    gpio_num_t prev_gpio;
    gpio_num_t next_gpio;
    uint16_t long_press_time_ms;
    uint16_t short_press_time_ms;
    button_short_press_cb_t on_short_press;
    button_long_press_cb_t on_long_press;
} buttons_config_t;

bool buttons_init(const buttons_config_t* config);

#ifdef __cplusplus
}
#endif
