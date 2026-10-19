#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "bme68x.h"
#include "bsec_interface_multi.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "sensing_types.h"

typedef struct {
    i2c_master_dev_handle_t device; // NULL while closed.
    struct bme68x_dev bme;
    uint8_t address;
} sensing_sensor_t;

typedef struct {
    void* instance; // NULL until opened; BSEC keeps its calibration here.
    int64_t next_call_ns;
} sensing_bsec_t;

// Finds the BME680 at 0x77 or 0x76 and initializes it.
esp_err_t sensing_sensor_open(i2c_master_bus_handle_t bus, sensing_sensor_t* sensor);
void sensing_sensor_close(sensing_sensor_t* sensor);

// Runs one forced-mode measurement with the settings BSEC asked for. *has_data is
// false when the sensor reported no new data.
esp_err_t sensing_sensor_measure(
    sensing_sensor_t* sensor, const bsec_bme_settings_t* settings, struct bme68x_data* data, bool* has_data);

// Creates the BSEC instance with the 3.3 V LP configuration and subscribes the outputs.
esp_err_t sensing_bsec_open(sensing_bsec_t* bsec);

// Lets BSEC control the sensor at its next_call and stores returned outputs in
// snapshot. *updated tells whether any output arrived.
esp_err_t sensing_bsec_step(
    sensing_bsec_t* bsec, sensing_sensor_t* sensor, sensing_snapshot_t* snapshot, bool* updated);
