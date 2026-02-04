/**
 * @file types.h
 * @brief Type definitions, enums, and structures for Mini OS v1
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>


// ============================================================
// SYSTEM STATE (FSM States)
// ============================================================
typedef enum {
  STATE_MAIN_MENU,
  STATE_MODE_MECANUM,
  STATE_MODE_RC,
  STATE_MODE_VOICE,
  STATE_MODE_SETTINGS,
} system_state_t;

// ============================================================
// SETTINGS MENU STATE
// ============================================================
typedef enum {
  SETTINGS_MAIN,
  SETTINGS_BRIGHTNESS,
  SETTINGS_VOLUME,
  SETTINGS_MOTOR_CAL,
  SETTINGS_MOTOR_TEST,
  SETTINGS_ABOUT,
} settings_menu_t;

// ============================================================
// MOVEMENT TYPES
// ============================================================
typedef enum {
  MOVEMENT_STOP,
  MOVEMENT_FORWARD,
  MOVEMENT_BACKWARD,
  MOVEMENT_STRAFE_LEFT,
  MOVEMENT_STRAFE_RIGHT,
  MOVEMENT_ROTATE_LEFT,
  MOVEMENT_ROTATE_RIGHT,
  MOVEMENT_TURN_LEFT,
  MOVEMENT_TURN_RIGHT,
  MOVEMENT_EMERGENCY,
} movement_type_t;

// ============================================================
// BUTTON EVENTS
// ============================================================
typedef enum {
  BTN_EVT_NONE,
  BTN_EVT_UP_PRESSED,
  BTN_EVT_DOWN_PRESSED,
  BTN_EVT_OK_SINGLE,
  BTN_EVT_OK_DOUBLE,
  BTN_EVT_OK_LONG,
} button_event_t;

// ============================================================
// SYSTEM EVENTS (for FreeRTOS queue)
// ============================================================
typedef enum {
  EVT_NONE,
  EVT_BUTTON,
  EVT_JOYSTICK_DATA,
  EVT_VOICE_CMD,
  EVT_TIMEOUT,
  EVT_STATE_CHANGE,
} event_type_t;

typedef struct {
  event_type_t type;
  union {
    button_event_t button;
    uint8_t voice_cmd;
    system_state_t new_state;
  } data;
} system_event_t;

// ============================================================
// JOYSTICK DATA (from slave)
// ============================================================
typedef struct {
  int16_t throttle; // -255 to 255 (Y axis)
  int16_t steering; // -255 to 255 (X axis)
  int16_t aux_x;    // unused
  int16_t aux_y;    // unused
  bool btn1;        // emergency stop
  bool btn2;        // unused
  uint8_t mode;     // unused
} joystick_data_t;

// ============================================================
// MOTOR SPEEDS
// ============================================================
typedef struct {
  int16_t fl; // Front Left
  int16_t fr; // Front Right
  int16_t bl; // Back Left
  int16_t br; // Back Right
} motor_speeds_t;

// ============================================================
// SETTINGS DATA
// ============================================================
typedef struct {
  uint8_t brightness;
  uint8_t volume;
  uint8_t motor_cal_fl;
  uint8_t motor_cal_fr;
  uint8_t motor_cal_bl;
  uint8_t motor_cal_br;
} settings_data_t;

// ============================================================
// SYSTEM CONTEXT (global state)
// ============================================================
typedef struct {
  // FSM state
  system_state_t current_state;
  settings_menu_t settings_menu;
  int8_t menu_index;
  int8_t settings_index;

  // Connection status
  bool joystick_connected;
  bool voice_connected;
  uint32_t last_joystick_time;
  uint32_t last_voice_time;

  // Joystick data
  joystick_data_t joystick;

  // Voice data
  uint8_t voice_cmd;
  uint8_t voice_speed;

  // Movement
  movement_type_t movement;
  motor_speeds_t motor_speeds;

  // Settings
  settings_data_t settings;

  // Display update flag
  bool display_dirty;
} system_context_t;

// Global context (extern declaration)
extern system_context_t g_ctx;

#endif // TYPES_H
