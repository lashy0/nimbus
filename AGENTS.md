# Nimbus v2

## Scope and current state

This branch is a clean implementation for LILYGO T-Display (ESP32, ST7789
135x240), BME680, and a 1150 mAh / 3.7 V battery in the onboard battery connector.
The old application and its components have been removed. Only runtime fonts and
images are reused from v1. Do not restore old application code as a starting point.

Read `docs/architecture.md` before implementing a component. Implementation status
and actual checks belong in `docs/verification-log.md`; electrical measurements
follow `docs/hardware-bringup.md`. Never present planned drivers as implemented.

## Toolchain and build

- ESP-IDF 5.5.3, target ESP32.
- Future UI: LVGL 8.4.0 with esp_lvgl_port.
- Future IAQ processing: Bosch BSEC2 2.6.1.0.
- Commit `dependencies.lock` when generated; pin external library revisions.

```powershell
. .\activate.ps1
idf.py build
idf.py -p COM3 monitor
```

COM3 is the port observed locally, not a portable setting. Flash with
`idf.py -p COM3 flash monitor` when hardware execution is part of the requested
work. Do not claim hardware verification from a build alone.

## Architecture

- `main`: initialization and wiring only.
- `app`: single owner of application state; events, navigation and policies.
- `board`: pins and low-level drivers for this board; only main depends on it.
- `sensing`: sole owner of BME680/BSEC lifecycle and measurement schedule.
- `power`: synchronous battery polling and MCU sleep commands; no task or display control.
- `storage`: settings and calibration persistence.
- `ui`: LVGL, panel sleep, actual brightness and display PM lock; accepts a plain model.

Main passes borrowed handles and callbacks with contexts to consumers. No service
includes board.h or depends on board through REQUIRES. Only app owns policies and
calls ui_set_display_state; only UI calls PWM/panel operations. App delivers models
synchronously through ui_apply_model with bounded LVGL locking, without a polling
delivery timer. Telemetry uses latest-value slots and notifications; user commands
use a queue, and shutdown ACK has a separate request-ID slot.

Only use new IDF drivers: driver/i2c_master.h, adc_oneshot/adc_cali, esp_lcd.
Transitions, navigation, policies, battery math and ui_model.h include no ESP-IDF,
FreeRTOS or LVGL headers.

Create component directories with their implementation. Do not add fake drivers
or success-returning placeholders. Keep state transitions separate from hardware APIs.

## Hardware assignments

Do not change pins unless requested:

- Display: MOSI19, SCLK18, CS5, DC16, RST23; backlight4.
- Display MISO must be -1: GPIO21 is I2C SDA.
- Buttons: PREV0, NEXT35; use board polarity, not boot-time autodetection.
- BME680: SDA21, SCL22, 3.3 V, I2C addresses 0x76/0x77.
- Battery ADC: GPIO34 / ADC1 channel6, ADC_EN14; verify actual measurements.

UI keeps ESP_PM_NO_LIGHT_SLEEP while ON or DIM. OFF must stop LVGL timers/render,
drain DMA, darken/sleep the panel and only then release its PM lock. On LVGL8,
lvgl_port_stop alone does not stop the task's periodic waits; implement the
documented quiesce/resume adaptation before claiming a sleeping UI.

Light-sleep uses level GPIO wake and consumes a held wake press. Deep-sleep uses
ext0 GPIO0 LOW plus ext1 GPIO35 ALL_LOW (single-pin mask), with both buttons
released before entry. Use verified RTC hold LOW for GPIO4/GPIO14 and safe release
on wake. Do not reset while GPIO0 is held; do not blanket-hold flash/UART pins.
ADC_EN is enabled only during a sample and disabled on every exit path.

Keep external power, charge state and battery percentage independent. Do not
claim charge completion or a valid percentage solely from USB or rising voltage.
Shutdown must use reliable electrical measurements, not only the smoothed UI SOC.

## LVGL and assets

All LVGL access outside its task requires lvgl_port_lock/unlock, including timer
creation. Timer callbacks already run inside the lock; do not acquire it again.
Only UI owns object pointers and deletes them. App models contain no LVGL types.

`assets/fonts/*.fnt` and `assets/img/**/*.bin` are retained unchanged from v1.
Original artwork is supplied by the user in `../Nimbus-assets` (38 JPG/PNG images
and two OTF fonts). Read originals there; do not modify that sibling directory.
See docs/assets.md for source gaps and import status. The current build must not
depend on the sibling directory being present.
Use runtime font loading and central image definitions when UI is implemented.
The intended filesystem is SPIFFS mounted at /spiffs, accessed as S:/spiffs/...
with LVGL 8.4.0, 16-bit color and the correct byte order. Runtime asset packaging
is not wired into the minimal build yet. No source files or build outputs belong
under assets; see docs/assets.md for the retained inventory.
SPIFFS/runtime fonts remain the baseline. EMBED_FILES or LittleFS require the
measured comparison described in the architecture, not a silent storage change.

BSEC phase mapping, LP config, state persistence and local-package policy are in
docs/bsec.md. Do not assume LP/ULP state compatibility or treat warning 14 as success.
BSEC binaries/config/headers are not covered by the Arduino wrapper's BSD license;
do not add them to the public repository before checking their separate terms.

## Verification

A hardware feature is verified only after a build without new warnings, a boot
without ESP_ERROR_CHECK aborts or LVGL asserts, and execution of its actual path
on the connected device. Report unavailable hardware and unverified paths plainly.
Inspect the earliest root error. Keep complete backtraces for decoding.
Core dumps are configured to flash in a 64 KiB partition; preserve matching ELF/map
for decoding and verify capacity after tasks exist. Task WDT panics after 10 s;
future app/sensing subscribe during active work and unsubscribe for deliberate
long waits, as documented, without periodic ULP wakeups merely to feed WDT.

Do not automatically erase NVS, format assets or reboot forever on recoverable
sensor/resource failures. A cached reading is not a fresh measurement. Pressure
passed to BSEC is in Pa. Observe BSEC next_call and calibration status.
