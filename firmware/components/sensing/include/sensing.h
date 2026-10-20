#pragma once

#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "sensing_types.h"

// Called from the sensing task after the snapshot changed; must not block.
typedef void (*sensing_notify_fn)(void* ctx);

typedef struct {
    i2c_master_bus_handle_t bus; // Borrowed; sensing adds and owns its device.
    sensing_notify_fn notify;
    void* ctx;
} sensing_config_t;

// Starts the sensing task, the only owner of the BME680 and BSEC. A missing or
// failing sensor is retried with backoff; the task never gives up.
esp_err_t sensing_start(const sensing_config_t* config);

// Stops for power-off: the task saves a changed BSEC state, confirms and parks
// for the rest of the boot. Blocks up to timeout_ms; ESP_ERR_TIMEOUT means no
// confirmation yet, and the task may still be writing. Call once.
esp_err_t sensing_stop(uint32_t timeout_ms);

// Copies the latest snapshot. Safe from any task.
void sensing_get_snapshot(sensing_snapshot_t* out);
