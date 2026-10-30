#pragma once

#include <stdbool.h>

#include "buttons_types.h"

/**
 * @file
 * The keyboard in place of the buttons: Left is PREV, Right is NEXT, held for
 * 1.5 s a long press. Letter keys go to sim_data_key().
 */

/** Call once, after the SDL window is created. */
void sim_input_start(void);

/** Takes the oldest press without waiting. */
bool sim_input_receive(buttons_event_t* out);
