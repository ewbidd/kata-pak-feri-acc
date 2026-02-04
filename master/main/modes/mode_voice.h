/**
 * @file mode_voice.h
 * @brief Voice control mode
 */

#ifndef MODE_VOICE_H
#define MODE_VOICE_H

#include "types.h"

void mode_voice_handle_button(button_event_t evt);
void mode_voice_process(void);
void mode_voice_draw(void);

#endif // MODE_VOICE_H
