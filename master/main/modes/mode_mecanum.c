/**
 * @file mode_mecanum.c
 * @brief Mecanum drive mode implementation
 *
 * Movement mapping for Mecanum wheels:
 *   FORWARD:      All motors forward
 *   BACKWARD:     All motors backward
 *   STRAFE LEFT:  FL+BR backward, FR+BL forward
 *   STRAFE RIGHT: FL+BR forward, FR+BL backward
 *   ROTATE LEFT:  FL+BL backward, FR+BR forward
 *   ROTATE RIGHT: FL+BL forward, FR+BR backward
 */

#include "esp_log.h"
#include <stdlib.h>


#include "buzzer.h"
#include "config.h"
#include "fsm.h"
#include "mode_mecanum.h"
#include "motor.h"
#include "types.h"
#include "ui_common.h"


static const char *TAG = "MECANUM";

// ============================================================
// BUTTON HANDLER
// ============================================================
void mode_mecanum_handle_button(button_event_t evt) {
  switch (evt) {
  case BTN_EVT_OK_DOUBLE:
    // Back to menu
    motor_stop_all();
    fsm_change_state(STATE_MAIN_MENU);
    break;

  case BTN_EVT_OK_LONG:
    // Emergency stop
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
// INTERPRET JOYSTICK FOR MECANUM
// ============================================================
static movement_type_t interpret_mecanum(int16_t throttle, int16_t steering) {
  // Apply deadzone
  if (abs(throttle) < DEADZONE)
    throttle = 0;
  if (abs(steering) < DEADZONE)
    steering = 0;

  // Emergency button check
  if (g_ctx.joystick.btn1) {
    return MOVEMENT_EMERGENCY;
  }

  // No input
  if (throttle == 0 && steering == 0) {
    return MOVEMENT_STOP;
  }

  // Check for strafe (horizontal movement only)
  if (abs(throttle) < DEADZONE && abs(steering) > DEADZONE) {
    return (steering > 0) ? MOVEMENT_STRAFE_RIGHT : MOVEMENT_STRAFE_LEFT;
  }

  // Check for rotation vs forward/backward
  float ratio = (float)abs(steering) / (float)(abs(throttle) + 1);

  if (abs(throttle) > DEADZONE) {
    if (ratio > DIAGONAL_RATIO) {
      // Rotation mode
      return (steering > 0) ? MOVEMENT_ROTATE_RIGHT : MOVEMENT_ROTATE_LEFT;
    } else {
      // Forward/backward
      return (throttle > 0) ? MOVEMENT_FORWARD : MOVEMENT_BACKWARD;
    }
  }

  return MOVEMENT_STOP;
}

// ============================================================
// CALCULATE MOTOR SPEEDS FOR MECANUM
// ============================================================
static void calculate_mecanum_speeds(int16_t throttle, int16_t steering,
                                     motor_speeds_t *speeds) {
  // Apply deadzone
  if (abs(throttle) < DEADZONE)
    throttle = 0;
  if (abs(steering) < DEADZONE)
    steering = 0;

  // Calculate base speed
  int16_t speed = abs(throttle);
  if (speed == 0)
    speed = abs(steering);
  if (speed > MAX_SPEED)
    speed = MAX_SPEED;

  switch (g_ctx.movement) {
  case MOVEMENT_FORWARD:
    speeds->fl = speed;
    speeds->fr = speed;
    speeds->bl = speed;
    speeds->br = speed;
    break;

  case MOVEMENT_BACKWARD:
    speeds->fl = -speed;
    speeds->fr = -speed;
    speeds->bl = -speed;
    speeds->br = -speed;
    break;

  case MOVEMENT_STRAFE_LEFT:
    speeds->fl = -speed;
    speeds->fr = speed;
    speeds->bl = speed;
    speeds->br = -speed;
    break;

  case MOVEMENT_STRAFE_RIGHT:
    speeds->fl = speed;
    speeds->fr = -speed;
    speeds->bl = -speed;
    speeds->br = speed;
    break;

  case MOVEMENT_ROTATE_LEFT:
    speeds->fl = -speed;
    speeds->fr = speed;
    speeds->bl = -speed;
    speeds->br = speed;
    break;

  case MOVEMENT_ROTATE_RIGHT:
    speeds->fl = speed;
    speeds->fr = -speed;
    speeds->bl = speed;
    speeds->br = -speed;
    break;

  case MOVEMENT_STOP:
  case MOVEMENT_EMERGENCY:
  default:
    speeds->fl = 0;
    speeds->fr = 0;
    speeds->bl = 0;
    speeds->br = 0;
    break;
  }
}

// ============================================================
// PROCESS
// ============================================================
void mode_mecanum_process(void) {
  // Interpret joystick
  movement_type_t new_movement =
      interpret_mecanum(g_ctx.joystick.throttle, g_ctx.joystick.steering);

  // Check for movement change
  if (new_movement != g_ctx.movement) {
    g_ctx.movement = new_movement;
    g_ctx.display_dirty = true;
    ESP_LOGI(TAG, "Movement: %d", new_movement);
  }

  // Calculate motor speeds
  calculate_mecanum_speeds(g_ctx.joystick.throttle, g_ctx.joystick.steering,
                           &g_ctx.motor_speeds);
}

// ============================================================
// DRAW
// ============================================================
void mode_mecanum_draw(void) {
  ui_draw_header("MECANUM");

  // Connection status
  if (!g_ctx.joystick_connected) {
    display_draw_string(15, 25, "Waiting for");
    display_draw_string(25, 35, "Joystick...");
  } else {
    // Show movement
    ui_draw_movement(g_ctx.movement);

    // Show joystick values
    char buf[32];
    snprintf(buf, sizeof(buf), "T:%4d S:%4d", g_ctx.joystick.throttle,
             g_ctx.joystick.steering);
    display_draw_string(10, 45, buf);
  }

  ui_draw_status_bar();
}
