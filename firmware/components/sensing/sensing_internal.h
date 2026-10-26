#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "bme68x.h"
#include "bsec_interface_multi.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "sensing_types.h"
#include "storage.h"

#define SENSING_PROFILE_LP 0            // storage_bsec_key_t.profile of the 3 s configuration.
#define SENSING_RESULT_TIMEOUT_MS 10000 // LP results arrive about every 3 s.

// BSEC progress stays private; consumers receive the resulting phase.
typedef struct {
    sensing_snapshot_t snapshot;
    bool stabilized; // BSEC stabilization status reached 1.
    bool run_in;     // BSEC run-in status reached 1.
} sensing_outputs_t;

typedef enum {
    SENSING_OUTPUT_IAQ = 1 << 0,
    SENSING_OUTPUT_TEMPERATURE = 1 << 1,
    SENSING_OUTPUT_HUMIDITY = 1 << 2,
    SENSING_OUTPUT_PRESSURE = 1 << 3,
} sensing_output_t;

#define SENSING_ENVIRONMENT_OUTPUTS (SENSING_OUTPUT_TEMPERATURE | SENSING_OUTPUT_HUMIDITY | SENSING_OUTPUT_PRESSURE)

typedef struct {
    i2c_master_dev_handle_t device; // NULL while closed.
    struct bme68x_dev bme;
    uint8_t address;
} sensing_sensor_t;

typedef struct {
    void* instance; // NULL until opened; BSEC keeps its calibration here.
    uint8_t* work;  // BSEC_MAX_WORKBUFFER_SIZE bytes, allocated with the instance.
    int64_t next_call_ns;
    storage_bsec_key_t key; // Library version and configuration of this instance.
} sensing_bsec_t;

// What is in storage, so the save policy can skip repeated milestones and
// unchanged states.
typedef struct {
    bool saved_calibrated; // A state with IAQ accuracy >= 1 is saved.
    bool saved_high;       // A state with IAQ accuracy 3 is saved.
    int64_t saved_ms;      // When storage last matched BSEC; INT64_MIN before that.
    int64_t attempt_ms;    // Last automatic save attempt; INT64_MIN before the first.
    bool record_unread;    // Storage could not be read at boot; its record may be good.
    uint8_t state[BSEC_MAX_STATE_BLOB_SIZE];
    size_t length; // 0 while nothing is saved.
} sensing_checkpoint_t;

// Finds the BME680 at 0x77 or 0x76 and initializes it.
esp_err_t sensing_sensor_open(i2c_master_bus_handle_t bus, sensing_sensor_t* sensor);
void sensing_sensor_close(sensing_sensor_t* sensor);

// Runs one forced-mode measurement with the settings BSEC asked for. *has_data is
// false when the sensor reported no new data.
esp_err_t sensing_sensor_measure(
    sensing_sensor_t* sensor,
    const bsec_bme_settings_t* settings,
    struct bme68x_data* data,
    bool* has_data);

// Creates the BSEC instance with the 3.3 V LP configuration, restores a matching
// saved state and subscribes the outputs.
esp_err_t sensing_bsec_open(sensing_bsec_t* bsec, sensing_checkpoint_t* checkpoint);

// Loads the saved state for key into instance, using the instance's work buffer,
// and describes it in checkpoint. ESP_FAIL means BSEC rejected the state and the
// instance must be set up again; any other error leaves the instance untouched.
esp_err_t sensing_state_restore(
    void* instance,
    uint8_t* work,
    const storage_bsec_key_t* key,
    sensing_checkpoint_t* checkpoint);

// Saves a changed BSEC state at power-off, outside the automatic schedule.
void sensing_state_save_final(
    const sensing_bsec_t* bsec,
    const sensing_snapshot_t* snapshot,
    sensing_checkpoint_t* checkpoint,
    int64_t now_ms);

// Saves the BSEC state when the save policy calls for it after new outputs.
void sensing_state_checkpoint(
    const sensing_bsec_t* bsec,
    const sensing_snapshot_t* snapshot,
    sensing_checkpoint_t* checkpoint,
    int64_t now_ms);

// Lets BSEC control the sensor at its next_call and stores returned outputs in
// outputs. *received identifies measurement values returned by this call;
// partial results are collected privately before publishing a complete snapshot.
esp_err_t sensing_bsec_step(
    sensing_bsec_t* bsec,
    sensing_sensor_t* sensor,
    sensing_outputs_t* outputs,
    uint8_t* received);
