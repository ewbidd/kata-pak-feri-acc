/**
 * @file fsm.h
 * @brief Finite State Machine core definitions
 */

#ifndef FSM_H
#define FSM_H

#include "types.h"

/**
 * @brief Initialize FSM
 */
void fsm_init(void);

/**
 * @brief Process button event
 * @param evt Button event type
 */
void fsm_process_button(button_event_t evt);

/**
 * @brief Process joystick data
 */
void fsm_process_joystick(void);

/**
 * @brief Process voice command
 */
void fsm_process_voice(void);

/**
 * @brief Change FSM state
 * @param new_state Target state
 */
void fsm_change_state(system_state_t new_state);

/**
 * @brief Update current mode (called from main loop)
 */
void fsm_update(void);

/**
 * @brief Get current state
 * @return Current system state
 */
system_state_t fsm_get_state(void);

#endif // FSM_H
