#pragma once

#include <stdbool.h>

void bme680_nvs_clear_state(void);
void bme680_nvs_load_state(void);
void bme680_nvs_save_state(bool force);
void bme680_nvs_save_state_on_progress(void);
