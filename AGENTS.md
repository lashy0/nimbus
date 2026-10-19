# Nimbus v2

## Scope and current state

This branch is a clean implementation for LILYGO T-Display (ESP32, ST7789
135x240), BME680, and a 1150 mAh / 3.7 V battery in the onboard battery connector.
The old application and its components have been removed. Only runtime fonts and
images are reused from v1. Do not restore old application code as a starting point.

Read `docs/architecture.md` before implementing a component. `main/app_main.c`
currently contains only a buildable entry point with startup logs; no board
drivers, UI, sensor service or power control have been implemented.

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
- `board`: pins and low-level drivers for this board.
- `sensing`: sole owner of BME680/BSEC lifecycle and measurement schedule.
- `power`: battery estimation and power commands; no UI or navigation.
- `storage`: settings and calibration persistence.
- `ui`: LVGL objects and resources; accepts a plain model, does not read sensors.

Create component directories with their implementation. Do not add fake drivers
or success-returning placeholders. Keep state transitions testable without ESP32.

## Hardware assignments

Do not change pins unless requested:

- Display: MOSI19, SCLK18, CS5, DC16, RST23; backlight4.
- Display MISO must be -1: GPIO21 is I2C SDA.
- Buttons: PREV0, NEXT35; use board polarity, not boot-time autodetection.
- BME680: SDA21, SCL22, 3.3 V, I2C addresses 0x76/0x77.
- Battery ADC: GPIO34 / ADC1 channel6, ADC_EN14; verify actual measurements.

Keep external power, charge state and battery percentage independent. Do not
claim charge completion or a valid percentage solely from USB or rising voltage.
Shutdown must use reliable electrical measurements, not only the smoothed UI SOC.

## LVGL and assets

All LVGL access outside its task requires lvgl_port_lock/unlock, including timer
creation. Timer callbacks already run inside the lock; do not acquire it again.
Only UI owns object pointers and deletes them. App models contain no LVGL types.

`assets/fonts/*.fnt` and `assets/img/**/*.bin` are retained unchanged from v1.
Use runtime font loading and central image definitions when UI is implemented.
The intended filesystem is SPIFFS mounted at /spiffs, accessed as S:/spiffs/...
with LVGL 8.4.0, 16-bit color and the correct byte order. Runtime asset packaging
is not wired into the minimal build yet. No source files or build outputs belong
under assets; see docs/assets.md for the retained inventory.

## Verification

A hardware feature is verified only after a build without new warnings, a boot
without ESP_ERROR_CHECK aborts or LVGL asserts, and execution of its actual path
on the connected device. Report unavailable hardware and untested paths plainly.
Inspect the earliest root error. Keep complete backtraces for decoding.

Do not automatically erase NVS, format assets or reboot forever on recoverable
sensor/resource failures. A cached reading is not a fresh measurement. Pressure
passed to BSEC is in Pa. Observe BSEC next_call and calibration status.
