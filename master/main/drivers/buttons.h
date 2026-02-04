/**
 * @file buttons.h
 * @brief Button handling with debounce and event detection
 */

#ifndef BUTTONS_H
#define BUTTONS_H

#include "types.h"

/**
 * @brief Initialize button GPIOs
 */
void buttons_init(void);

/**
 * @brief Poll buttons and return event
 * @return Button event (or BTN_EVT_NONE)
 */
button_event_t buttons_poll(void);

#endif // BUTTONS_H
