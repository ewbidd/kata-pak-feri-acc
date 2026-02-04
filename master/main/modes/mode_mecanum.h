/**
 * @file mode_mecanum.h
 * @brief Mecanum drive mode
 */

#ifndef MODE_MECANUM_H
#define MODE_MECANUM_H

#include "types.h"

void mode_mecanum_handle_button(button_event_t evt);
void mode_mecanum_process(void);
void mode_mecanum_draw(void);

#endif // MODE_MECANUM_H
