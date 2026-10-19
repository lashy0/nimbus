#pragma once

#include "esp_err.h"

// Posts synthetic readings to app in a loop so every screen state can be checked on
// the device before sensing and power exist. Call after app_start().
esp_err_t preview_readings_start(void);
