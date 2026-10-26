#pragma once

/**
 * Prepares final deep sleep and never returns.
 *
 * Call from task context; waits for panel reset and up to 3 s for button release.
 * Safe while LVGL still flushes: GPIO4 and ADC_EN are held LOW on the RTC mux,
 * disconnecting LEDC, and a hardware reset puts ST7789 into Sleep In with reset
 * held HIGH. A button must stay released for 50 ms to arm its wake source;
 * buttons still held after the wait do not arm wake. PREV uses EXT0, NEXT EXT1.
 * Pin preparation errors are logged and sleep proceeds.
 *
 * @param ctx Unused; matches the power sleep callback.
 */
void board_enter_deep_sleep(void* ctx);
