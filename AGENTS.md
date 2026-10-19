# Nimbus v2

## Scope and current state

This branch is a clean implementation for LILYGO T-Display (ESP32, ST7789
135x240), BME680, and a 1150 mAh / 3.7 V battery in the onboard battery connector.
The old application and its components have been removed. Only runtime fonts and
images are reused from v1. Do not restore old application code as a starting point.
Initial board diagnostics exist; full application services are still pending.

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
Use the matching build directory/ELF for monitoring; board diagnostics commands
are in docs/hardware-bringup.md.

## Architecture

- `main`: initialization and wiring only.
- `app`: single owner of application state; events, navigation and policies.
- `board`: pins and low-level drivers for this board; only main depends on it.
- `sensing`: sole owner of BME680/BSEC lifecycle and measurement schedule.
- `power`: synchronous battery polling and MCU sleep commands; no task or display control.
- `storage`: settings and calibration persistence.
- `ui`: LVGL, panel sleep, actual brightness and display PM lock; accepts a plain model.

Main passes borrowed handles and callbacks with contexts; no service depends on
board. Follow the ownership and display contracts in
[architecture sections 2–5](docs/architecture.md).

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

Keep hardware transitions, button ownership and wake/hold sequencing consistent
with [architecture sections 4–9](docs/architecture.md).
Power validity and startup/safe-mode rules have one source of truth in
[sections 8–11](docs/architecture.md).

## LVGL and assets

Only UI owns LVGL objects; follow its locking and lifecycle contract in
[architecture section 4](docs/architecture.md).

`assets/fonts/*.fnt` and `assets/img/**/*.bin` are retained unchanged from v1.
Original artwork is supplied by the user in `../Nimbus-assets` (38 JPG/PNG images
and two OTF fonts). Read originals there; do not modify that sibling directory.
See docs/assets.md for source gaps and import status. The current build must not
depend on the sibling directory being present.
Runtime asset packaging is not wired into the minimal build yet. Follow
[assets.md](docs/assets.md) and
[architecture section 10](docs/architecture.md) for
formats, storage and memory decisions; no build outputs belong under assets.

BSEC phases, profiles, persistence, mandatory-package build policy and license
constraints have one source of truth in [bsec.md](docs/bsec.md).

## Verification

A hardware feature is verified only after a build without new warnings, a boot
without ESP_ERROR_CHECK aborts or LVGL asserts, and execution of its actual path
on the connected device. Report unavailable hardware and unverified paths plainly.
Inspect the earliest root error. Keep complete backtraces for decoding.
Preserve matching ELF/map for core dumps. Follow the watchdog and crash-recovery
rules in [architecture section 5](docs/architecture.md)
and [section 11](docs/architecture.md).

Do not automatically erase NVS, format assets or reboot forever on recoverable
sensor/resource failures. A cached reading is not a fresh measurement. Pressure
passed to BSEC is in Pa. Observe BSEC next_call and calibration status.
