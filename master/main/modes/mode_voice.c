/**
 * @file mode_voice.c
 * @brief Voice control mode implementation
 *
 * Commands from voice slave:
 *   0x00 = STOP
 *   0x01 = FORWARD
 *   0x02 = BACKWARD
 *   0x03 = LEFT (turn)
 *   0x04 = RIGHT (turn)
 */

#include "esp_log.h"

#include "buzzer.h"
#include "config.h"
#include "fsm.h"
#include "mode_voice.h"
#include "motor.h"
#include "types.h"
#include "ui_common.h"


static const char *TAG = "VOICE";

// ============================================================
// BUTTON HANDLER
// ============================================================
void mode_voice_handle_button(button_event_t evt) {
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
// PROCESS VOICE COMMAND
// ============================================================
void mode_voice_process(void) {
  movement_type_t new_movement = MOVEMENT_STOP;
  int16_t speed = g_ctx.voice_speed;

  switch (g_ctx.voice_cmd) {
  case VOICE_CMD_STOP:
    new_movement = MOVEMENT_STOP;
    break;
  case VOICE_CMD_FORWARD:
    new_movement = MOVEMENT_FORWARD;
    break;
  case VOICE_CMD_BACKWARD:
    new_movement = MOVEMENT_BACKWARD;
    break;
  case VOICE_CMD_LEFT:
    new_movement = MOVEMENT_ROTATE_LEFT;
    break;
  case VOICE_CMD_RIGHT:
    new_movement = MOVEMENT_ROTATE_RIGHT;
    break;
  default:
    new_movement = MOVEMENT_STOP;
    break;
  }

  if (new_movement != g_ctx.movement) {
    g_ctx.movement = new_movement;
    g_ctx.display_dirty = true;
    ESP_LOGI(TAG, "Voice: %d -> Movement: %d", g_ctx.voice_cmd, new_movement);
  }

  // Calculate motor speeds based on movement and voice speed
  switch (g_ctx.movement) {
  case MOVEMENT_FORWARD:
    g_ctx.motor_speeds.fl = speed;
    g_ctx.motor_speeds.fr = speed;
    g_ctx.motor_speeds.bl = speed;
    g_ctx.motor_speeds.br = speed;
    break;

  case MOVEMENT_BACKWARD:
    g_ctx.motor_speeds.fl = -speed;
    g_ctx.motor_speeds.fr = -speed;
    g_ctx.motor_speeds.bl = -speed;
    g_ctx.motor_speeds.br = -speed;
    break;

  case MOVEMENT_ROTATE_LEFT:
    g_ctx.motor_speeds.fl = -speed;
    g_ctx.motor_speeds.fr = speed;
    g_ctx.motor_speeds.bl = -speed;
    g_ctx.motor_speeds.br = speed;
    break;

  case MOVEMENT_ROTATE_RIGHT:
    g_ctx.motor_speeds.fl = speed;
    g_ctx.motor_speeds.fr = -speed;
    g_ctx.motor_speeds.bl = speed;
    g_ctx.motor_speeds.br = -speed;
    break;

  case MOVEMENT_STOP:
  case MOVEMENT_EMERGENCY:
  default:
    g_ctx.motor_speeds.fl = 0;
    g_ctx.motor_speeds.fr = 0;
    g_ctx.motor_speeds.bl = 0;
    g_ctx.motor_speeds.br = 0;
    break;
  }
}

// ============================================================
// GET COMMAND STRING
// ============================================================
static const char *get_voice_cmd_str(uint8_t cmd) {
  switch (cmd) {
  case VOICE_CMD_STOP:
    return "STOP";
  case VOICE_CMD_FORWARD:
    return "FORWARD";
  case VOICE_CMD_BACKWARD:
    return "BACKWARD";
  case VOICE_CMD_LEFT:
    return "LEFT";
  case VOICE_CMD_RIGHT:
    return "RIGHT";
  default:
    return "UNKNOWN";
  }
}

// ============================================================
// DRAW
// ============================================================
void mode_voice_draw(void) {
  ui_draw_header("VOICE");

  if (!g_ctx.voice_connected) {
    display_draw_string(15, 25, "Waiting for");
    display_draw_string(18, 35, "Voice Slave...");
  } else {
    // Show current command
    display_draw_string(30, 18, "Command:");

    const char *cmd_str = get_voice_cmd_str(g_ctx.voice_cmd);
    int x = (OLED_WIDTH - strlen(cmd_str) * 6) / 2;
    display_draw_string(x, 30, cmd_str);

    // Show speed
    char buf[16];
    snprintf(buf, sizeof(buf), "Speed: %d", g_ctx.voice_speed);
    display_draw_string(30, 45, buf);
  }

  ui_draw_status_bar();
}
