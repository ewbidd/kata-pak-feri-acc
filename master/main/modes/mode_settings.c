/**
 * @file mode_settings.c
 * @brief Settings menu mode implementation
 */

#include "esp_log.h"
#include <stdio.h>


#include "buzzer.h"
#include "config.h"
#include "display.h"
#include "fsm.h"
#include "mode_settings.h"
#include "motor.h"
#include "nvs_storage.h"
#include "types.h"
#include "ui_common.h"


static const char *TAG = "SETTINGS";

#define SETTINGS_ITEMS 5

static const char *s_settings_items[] = {
    "Brightness", "Volume", "Motor Calibration", "Motor Test", "Save & Exit"};

// Sub-menu state
static int8_t s_motor_test_id = 0; // 0-3 for FL/FR/BL/BR, 4 for ALL
static bool s_motor_running = false;

// ============================================================
// BUTTON HANDLER - MAIN SETTINGS
// ============================================================
static void handle_main_menu(button_event_t evt) {
  switch (evt) {
  case BTN_EVT_UP_PRESSED:
    if (g_ctx.settings_index > 0) {
      g_ctx.settings_index--;
      buzzer_click();
    }
    break;

  case BTN_EVT_DOWN_PRESSED:
    if (g_ctx.settings_index < SETTINGS_ITEMS - 1) {
      g_ctx.settings_index++;
      buzzer_click();
    }
    break;

  case BTN_EVT_OK_SINGLE:
    switch (g_ctx.settings_index) {
    case 0:
      g_ctx.settings_menu = SETTINGS_BRIGHTNESS;
      break;
    case 1:
      g_ctx.settings_menu = SETTINGS_VOLUME;
      break;
    case 2:
      g_ctx.settings_menu = SETTINGS_MOTOR_CAL;
      break;
    case 3:
      g_ctx.settings_menu = SETTINGS_MOTOR_TEST;
      s_motor_test_id = 0;
      s_motor_running = false;
      break;
    case 4:
      // Save & Exit
      nvs_storage_save(&g_ctx.settings);
      motor_set_calibration(
          g_ctx.settings.motor_cal_fl, g_ctx.settings.motor_cal_fr,
          g_ctx.settings.motor_cal_bl, g_ctx.settings.motor_cal_br);
      display_set_brightness(g_ctx.settings.brightness);
      buzzer_set_volume(g_ctx.settings.volume);
      fsm_change_state(STATE_MAIN_MENU);
      return;
    }
    buzzer_click();
    break;

  case BTN_EVT_OK_DOUBLE:
    // Exit without saving
    fsm_change_state(STATE_MAIN_MENU);
    break;

  default:
    break;
  }
  g_ctx.display_dirty = true;
}

// ============================================================
// BUTTON HANDLER - BRIGHTNESS
// ============================================================
static void handle_brightness(button_event_t evt) {
  switch (evt) {
  case BTN_EVT_UP_PRESSED:
    if (g_ctx.settings.brightness < 245) {
      g_ctx.settings.brightness += 10;
    } else {
      g_ctx.settings.brightness = 255;
    }
    display_set_brightness(g_ctx.settings.brightness);
    buzzer_click();
    break;

  case BTN_EVT_DOWN_PRESSED:
    if (g_ctx.settings.brightness > 10) {
      g_ctx.settings.brightness -= 10;
    } else {
      g_ctx.settings.brightness = 5;
    }
    display_set_brightness(g_ctx.settings.brightness);
    buzzer_click();
    break;

  case BTN_EVT_OK_SINGLE:
  case BTN_EVT_OK_DOUBLE:
    g_ctx.settings_menu = SETTINGS_MAIN;
    buzzer_click();
    break;

  default:
    break;
  }
  g_ctx.display_dirty = true;
}

// ============================================================
// BUTTON HANDLER - VOLUME
// ============================================================
static void handle_volume(button_event_t evt) {
  switch (evt) {
  case BTN_EVT_UP_PRESSED:
    if (g_ctx.settings.volume < 95) {
      g_ctx.settings.volume += 5;
    } else {
      g_ctx.settings.volume = 100;
    }
    buzzer_set_volume(g_ctx.settings.volume);
    buzzer_click();
    break;

  case BTN_EVT_DOWN_PRESSED:
    if (g_ctx.settings.volume > 5) {
      g_ctx.settings.volume -= 5;
    } else {
      g_ctx.settings.volume = 0;
    }
    buzzer_set_volume(g_ctx.settings.volume);
    if (g_ctx.settings.volume > 0)
      buzzer_click();
    break;

  case BTN_EVT_OK_SINGLE:
  case BTN_EVT_OK_DOUBLE:
    g_ctx.settings_menu = SETTINGS_MAIN;
    buzzer_click();
    break;

  default:
    break;
  }
  g_ctx.display_dirty = true;
}

// ============================================================
// BUTTON HANDLER - MOTOR CALIBRATION
// ============================================================
static uint8_t *get_current_cal(void) {
  switch (g_ctx.settings_index) {
  case 0:
    return &g_ctx.settings.motor_cal_fl;
  case 1:
    return &g_ctx.settings.motor_cal_fr;
  case 2:
    return &g_ctx.settings.motor_cal_bl;
  case 3:
    return &g_ctx.settings.motor_cal_br;
  default:
    return NULL;
  }
}

static void handle_motor_cal(button_event_t evt) {
  switch (evt) {
  case BTN_EVT_UP_PRESSED:
    if (g_ctx.settings_index < 4) {
      // Adjust calibration value
      uint8_t *cal = get_current_cal();
      if (cal && *cal < 245)
        *cal += 10;
      else if (cal)
        *cal = 255;
    }
    buzzer_click();
    break;

  case BTN_EVT_DOWN_PRESSED:
    if (g_ctx.settings_index < 4) {
      uint8_t *cal = get_current_cal();
      if (cal && *cal > 10)
        *cal -= 10;
      else if (cal)
        *cal = 0;
    }
    buzzer_click();
    break;

  case BTN_EVT_OK_SINGLE:
    // Next motor or back
    if (g_ctx.settings_index < 3) {
      g_ctx.settings_index++;
    } else {
      g_ctx.settings_index = 0;
      g_ctx.settings_menu = SETTINGS_MAIN;
    }
    buzzer_click();
    break;

  case BTN_EVT_OK_DOUBLE:
    g_ctx.settings_index = 0;
    g_ctx.settings_menu = SETTINGS_MAIN;
    buzzer_click();
    break;

  default:
    break;
  }
  g_ctx.display_dirty = true;
}

// ============================================================
// BUTTON HANDLER - MOTOR TEST
// ============================================================
static void handle_motor_test(button_event_t evt) {
  switch (evt) {
  case BTN_EVT_UP_PRESSED:
    if (s_motor_test_id > 0) {
      s_motor_test_id--;
    }
    if (s_motor_running) {
      motor_stop_all();
      s_motor_running = false;
    }
    buzzer_click();
    break;

  case BTN_EVT_DOWN_PRESSED:
    if (s_motor_test_id < 4) { // 0-3 = individual, 4 = ALL
      s_motor_test_id++;
    }
    if (s_motor_running) {
      motor_stop_all();
      s_motor_running = false;
    }
    buzzer_click();
    break;

  case BTN_EVT_OK_SINGLE:
    // Toggle motor
    if (s_motor_running) {
      motor_stop_all();
      s_motor_running = false;
    } else {
      if (s_motor_test_id == 4) {
        // Test all motors
        motor_speeds_t speeds = {150, 150, 150, 150};
        motor_apply_speeds(&speeds);
      } else {
        motor_test(s_motor_test_id, 150);
      }
      s_motor_running = true;
    }
    buzzer_click();
    break;

  case BTN_EVT_OK_DOUBLE:
    motor_stop_all();
    s_motor_running = false;
    g_ctx.settings_menu = SETTINGS_MAIN;
    buzzer_click();
    break;

  default:
    break;
  }
  g_ctx.display_dirty = true;
}

// ============================================================
// MAIN BUTTON HANDLER
// ============================================================
void mode_settings_handle_button(button_event_t evt) {
  switch (g_ctx.settings_menu) {
  case SETTINGS_MAIN:
    handle_main_menu(evt);
    break;
  case SETTINGS_BRIGHTNESS:
    handle_brightness(evt);
    break;
  case SETTINGS_VOLUME:
    handle_volume(evt);
    break;
  case SETTINGS_MOTOR_CAL:
    handle_motor_cal(evt);
    break;
  case SETTINGS_MOTOR_TEST:
    handle_motor_test(evt);
    break;
  default:
    break;
  }
}

// ============================================================
// DRAW FUNCTIONS
// ============================================================
static void draw_main_menu(void) {
  ui_draw_header("SETTINGS");

  for (int i = 0; i < SETTINGS_ITEMS; i++) {
    int y = 14 + i * 10;
    ui_draw_menu_item(y, s_settings_items[i], i == g_ctx.settings_index);
  }
}

static void draw_brightness(void) {
  ui_draw_header("BRIGHTNESS");

  char buf[16];
  snprintf(buf, sizeof(buf), "%d", g_ctx.settings.brightness);
  display_draw_string(50, 25, buf);

  ui_draw_progress_bar(10, 40, 108, 12, g_ctx.settings.brightness);
}

static void draw_volume(void) {
  ui_draw_header("VOLUME");

  char buf[16];
  snprintf(buf, sizeof(buf), "%d%%", g_ctx.settings.volume);
  display_draw_string(50, 25, buf);

  uint8_t bar_val = (g_ctx.settings.volume * 255) / 100;
  ui_draw_progress_bar(10, 40, 108, 12, bar_val);
}

static void draw_motor_cal(void) {
  ui_draw_header("MOTOR CAL");

  const char *names[] = {"FL", "FR", "BL", "BR"};
  uint8_t *vals[] = {&g_ctx.settings.motor_cal_fl, &g_ctx.settings.motor_cal_fr,
                     &g_ctx.settings.motor_cal_bl,
                     &g_ctx.settings.motor_cal_br};

  for (int i = 0; i < 4; i++) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%s: %d", names[i], *vals[i]);
    int y = 16 + i * 11;
    bool sel = (i == g_ctx.settings_index);
    if (sel) {
      display_draw_string(4, y, ">");
    }
    display_draw_string(14, y, buf);
  }
}

static void draw_motor_test(void) {
  ui_draw_header("MOTOR TEST");

  const char *names[] = {"FL", "FR", "BL", "BR", "ALL"};

  for (int i = 0; i < 5; i++) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%s", names[i]);
    int y = 16 + i * 9;
    bool sel = (i == s_motor_test_id);
    if (sel) {
      display_draw_string(4, y, ">");
    }
    display_draw_string(14, y, buf);
  }

  // Status
  if (s_motor_running) {
    display_draw_string(70, 30, "RUNNING");
  } else {
    display_draw_string(70, 30, "STOPPED");
  }
}

void mode_settings_draw(void) {
  switch (g_ctx.settings_menu) {
  case SETTINGS_MAIN:
    draw_main_menu();
    break;
  case SETTINGS_BRIGHTNESS:
    draw_brightness();
    break;
  case SETTINGS_VOLUME:
    draw_volume();
    break;
  case SETTINGS_MOTOR_CAL:
    draw_motor_cal();
    break;
  case SETTINGS_MOTOR_TEST:
    draw_motor_test();
    break;
  default:
    draw_main_menu();
    break;
  }
}
