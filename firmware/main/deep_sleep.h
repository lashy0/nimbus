#pragma once

/**
 * @file
 * Deep sleep in place of power-off; a button press starts the firmware again.
 */

typedef enum {
    DEEP_SLEEP_USER,      /**< Confirmed on screen. */
    DEEP_SLEEP_CRITICAL,  /**< The battery is flat. */
    DEEP_SLEEP_LOW_START, /**< Too low to start the display and the sensor. */
    DEEP_SLEEP_CRASHES,   /**< The firmware keeps crashing. */
} deep_sleep_reason_t;

/**
 * Releases the pins held through the last deep sleep and logs why it began.
 *
 * Call first at boot, before any component sets up its pins.
 */
void deep_sleep_restore(void);

/**
 * Holds the backlight and the ADC divider off, resets the panel into sleep and
 * sleeps until a button press. Waits up to 3 s for both buttons to be released;
 * one still held does not wake the device. Pin errors are logged and sleep goes on.
 *
 * Call from a task once the UI and sensing are stopped.
 */
__attribute__((noreturn)) void deep_sleep_enter(deep_sleep_reason_t reason);
