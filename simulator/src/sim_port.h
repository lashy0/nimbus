#pragma once

/**
 * @file
 * The screen hardware of ui_port.h: a black shade over the window stands for
 * the backlight. The brightness is only logged as saved; each start has the default.
 */

/** Call once, after the SDL window is created and before the view. */
void sim_port_create(void);

/** Darkens the screen for good, like the device powering off. */
void sim_port_power_off(void);
