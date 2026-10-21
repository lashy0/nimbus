#pragma once

#include "esp_err.h"

// Brings up board resources and wires them to their consumers. An error means the
// device cannot work; a failing sensor, storage or assets are not errors here.
esp_err_t composition_init(void);

// Final sleep after composition_init() failed, with the backlight LOW and the panel
// reset; a button press boots again.
__attribute__((noreturn)) void composition_enter_fault_sleep(void);
