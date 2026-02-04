/**
 * @file mode_rc.c
 * @brief RC car mode (differential drive) implementation
 *
 * Movement mapping for RC car (no strafe):
 *   FORWARD:    All motors forward
 *   BACKWARD:   All motors backward
 *   TURN LEFT:  Left motors slower/reverse, right motors forward
 *   TURN RIGHT: Right motors slower/reverse, left motors forward
 */

#include "esp_log.h"
#include <stdlib.h>


#include "buzzer.h"
#include "config.h"
#include "fsm.h"
#include "mode_rc.h"
#include "motor.h"
#include "types.h"
#include "ui_common.h"


static const char *TAG = "RC";

// ============================================================
// BUTTON HANDLER
// ============================================================
void mode_rc_handle_button(button_event_t evt) {
  switch (evt) {
  case BTN_EVT_OK_DOUBLE:
    motor_stop_all();
    fsm_change_state(STATE_MAIN_MENU);
    break;

  case BTN_EVT_OK_LONG:
    motor_stop_all();
    g_ctx.movement = MOVEMENT_EMERGENCY;
    g_ctx.display_dirty = true;
    buzzer_error();
    break;

  default:
    break;
  }
}

// ============================================================
// INTERPRET JOYSTICK FOR RC
// ============================================================
static movement_type_t interpret_rc(int16_t throttle, int16_t steering) {
  if (abs(throttle) < DEADZONE)
    throttle = 0;
  if (abs(steering) < DEADZONE)
    steering = 0;

  if (g_ctx.joystick.btn1) {
    return MOVEMENT_EMERGENCY;
  }

  if (throttle == 0 && steering == 0) {
    return MOVEMENT_STOP;
  }

  // RC mode: throttle controls speed, steering controls direction
  if (abs(throttle) > DEADZONE) {
    if (abs(steering) > DEADZONE) {
      // Turning while moving
      if (throttle > 0) {
        return (steering > 0) ? MOVEMENT_TURN_RIGHT : MOVEMENT_TURN_LEFT;
      } else {
        // Reverse turning (inverted)
        return (steering > 0) ? MOVEMENT_TURN_LEFT : MOVEMENT_TURN_RIGHT;
      }
    } else {
      return (throttle > 0) ? MOVEMENT_FORWARD : MOVEMENT_BACKWARD;
    }
  }

  // Steering without throttle: rotate in place
  if (abs(steering) > DEADZONE) {
    return (steering > 0) ? MOVEMENT_ROTATE_RIGHT : MOVEMENT_ROTATE_LEFT;
  }

  return MOVEMENT_STOP;
}

// ============================================================
// CALCULATE MOTOR SPEEDS FOR RC (DIFFERENTIAL DRIVE)
// ============================================================
static void calculate_rc_speeds(int16_t throttle, int16_t steering,
                                motor_speeds_t *speeds) {
  if (abs(throttle) < DEADZONE)
    throttle = 0;
  if (abs(steering) < DEADZONE)
    steering = 0;

  // Tank/differential mixing
  int16_t left_speed = throttle + steering;
  int16_t right_speed = throttle - steering;

  // Clamp speeds
  if (left_speed > MAX_SPEED)
    left_speed = MAX_SPEED;
  if (left_speed < -MAX_SPEED)
    left_speed = -MAX_SPEED;
  if (right_speed > MAX_SPEED)
    right_speed = MAX_SPEED;
  if (right_speed < -MAX_SPEED)
    right_speed = -MAX_SPEED;

  // Apply to all 4 motors (left side = FL+BL, right side = FR+BR)
  speeds->fl = left_speed;
  speeds->bl = left_speed;
  speeds->fr = right_speed;
  speeds->br = right_speed;

  // Emergency stop
  if (g_ctx.movement == MOVEMENT_EMERGENCY) {
    speeds->fl = 0;
    speeds->fr = 0;
    speeds->bl = 0;
    speeds->br = 0;
  }
}

// ============================================================
// PROCESS
// ============================================================
void mode_rc_process(void) {
  movement_type_t new_movement =
      interpret_rc(g_ctx.joystick.throttle, g_ctx.joystick.steering);

  if (new_movement != g_ctx.movement) {
    g_ctx.movement = new_movement;
    g_ctx.display_dirty = true;
    ESP_LOGI(TAG, "Movement: %d", new_movement);
  }

  calculate_rc_speeds(g_ctx.joystick.throttle, g_ctx.joystick.steering,
                      &g_ctx.motor_speeds);
}

// ============================================================
// DRAW
// ============================================================
void mode_rc_draw(void) {
  ui_draw_header("RC MODE");

  if (!g_ctx.joystick_connected) {
    display_draw_string(15, 25, "Waiting for");
    display_draw_string(25, 35, "Joystick...");
  } else {
    ui_draw_movement(g_ctx.movement);

    char buf[32];
    snprintf(buf, sizeof(buf), "T:%4d S:%4d", g_ctx.joystick.throttle,
             g_ctx.joystick.steering);
    display_draw_string(10, 45, buf);
  }

  ui_draw_status_bar();
}
