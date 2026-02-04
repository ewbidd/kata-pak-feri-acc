/**
 * @file fsm.c
 * @brief Finite State Machine core implementation
 */

#include "fsm.h"
#include "buzzer.h"
#include "config.h"
#include "display.h"
#include "esp_log.h"
#include "mode_mecanum.h"
#include "mode_menu.h"
#include "mode_rc.h"
#include "mode_settings.h"
#include "mode_voice.h"
#include "motor.h"


static const char *TAG = "FSM";

// ============================================================
// INITIALIZATION
// ============================================================
void fsm_init(void) {
  g_ctx.current_state = STATE_MAIN_MENU;
  g_ctx.menu_index = 0;
  g_ctx.settings_index = 0;
  g_ctx.display_dirty = true;

  ESP_LOGI(TAG, "FSM initialized, starting in MAIN_MENU");
}

// ============================================================
// STATE CHANGE
// ============================================================
void fsm_change_state(system_state_t new_state) {
  if (g_ctx.current_state == new_state) {
    return;
  }

  // Exit current state
  switch (g_ctx.current_state) {
  case STATE_MODE_MECANUM:
  case STATE_MODE_RC:
  case STATE_MODE_VOICE:
    motor_stop_all();
    break;
  default:
    break;
  }

  ESP_LOGI(TAG, "State change: %d -> %d", g_ctx.current_state, new_state);

  g_ctx.current_state = new_state;
  g_ctx.display_dirty = true;

  // Enter new state
  switch (new_state) {
  case STATE_MAIN_MENU:
    g_ctx.menu_index = 0;
    break;
  case STATE_MODE_SETTINGS:
    g_ctx.settings_menu = SETTINGS_MAIN;
    g_ctx.settings_index = 0;
    break;
  default:
    break;
  }

  buzzer_click();
}

// ============================================================
// PROCESS BUTTON EVENT
// ============================================================
void fsm_process_button(button_event_t evt) {
  if (evt == BTN_EVT_NONE)
    return;

  switch (g_ctx.current_state) {
  case STATE_MAIN_MENU:
    mode_menu_handle_button(evt);
    break;
  case STATE_MODE_MECANUM:
    mode_mecanum_handle_button(evt);
    break;
  case STATE_MODE_RC:
    mode_rc_handle_button(evt);
    break;
  case STATE_MODE_VOICE:
    mode_voice_handle_button(evt);
    break;
  case STATE_MODE_SETTINGS:
    mode_settings_handle_button(evt);
    break;
  }
}

// ============================================================
// PROCESS JOYSTICK DATA
// ============================================================
void fsm_process_joystick(void) {
  switch (g_ctx.current_state) {
  case STATE_MODE_MECANUM:
    mode_mecanum_process();
    break;
  case STATE_MODE_RC:
    mode_rc_process();
    break;
  default:
    break;
  }
}

// ============================================================
// PROCESS VOICE COMMAND
// ============================================================
void fsm_process_voice(void) {
  if (g_ctx.current_state == STATE_MODE_VOICE) {
    mode_voice_process();
  }
}

// ============================================================
// FSM UPDATE (called from main loop)
// ============================================================
void fsm_update(void) {
  // Check connection timeouts
  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

  if (g_ctx.joystick_connected &&
      (now - g_ctx.last_joystick_time > CONNECTION_TIMEOUT_MS)) {
    g_ctx.joystick_connected = false;
    if (g_ctx.current_state == STATE_MODE_MECANUM ||
        g_ctx.current_state == STATE_MODE_RC) {
      g_ctx.movement = MOVEMENT_STOP;
      motor_stop_all();
    }
    g_ctx.display_dirty = true;
    ESP_LOGW(TAG, "Joystick disconnected");
  }

  if (g_ctx.voice_connected &&
      (now - g_ctx.last_voice_time > CONNECTION_TIMEOUT_MS)) {
    g_ctx.voice_connected = false;
    if (g_ctx.current_state == STATE_MODE_VOICE) {
      g_ctx.movement = MOVEMENT_STOP;
      motor_stop_all();
    }
    g_ctx.display_dirty = true;
    ESP_LOGW(TAG, "Voice slave disconnected");
  }
}

// ============================================================
// GET CURRENT STATE
// ============================================================
system_state_t fsm_get_state(void) { return g_ctx.current_state; }
