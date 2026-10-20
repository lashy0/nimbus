#pragma once

// Final deep sleep; never returns. Only GPIO work, so it is safe while the LVGL
// task still flushes: GPIO4 and ADC_EN are held LOW on the RTC mux, which cuts the
// LEDC signal, and a hardware reset puts the ST7789 into Sleep In with its reset
// line held HIGH. Then it waits up to 3 s for both buttons to stay released for
// 50 ms; a button still held after that does not arm its wake source, so a stuck
// button cannot wake the device at once. PREV wakes through EXT0, NEXT through EXT1.
// ctx is unused; the signature matches the power sleep callback.
void board_enter_deep_sleep(void* ctx);
