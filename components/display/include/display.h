#pragma once

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIDTH   135
#define HEIGHT  240

#define PIN_NUM_BL 4

lv_disp_t* display_init_lvgl(void);

#ifdef __cplusplus
}
#endif