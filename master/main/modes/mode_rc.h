/**
 * @file mode_rc.h
 * @brief RC car mode (differential drive)
 */

#ifndef MODE_RC_H
#define MODE_RC_H

#include "types.h"

void mode_rc_handle_button(button_event_t evt);
void mode_rc_process(void);
void mode_rc_draw(void);

#endif // MODE_RC_H
