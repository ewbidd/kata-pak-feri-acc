/**
 * @file mode_menu.c
 * @brief Main menu mode implementation
 */

#include "mode_menu.h"
#include "buzzer.h"
#include "config.h"
#include "fsm.h"
#include "types.h"
#include "ui_common.h"


#define MENU_ITEMS 4

static const char *s_menu_items[] = {"Mecanum Mode", "RC Mode", "Voice Mode",
                                     "Settings"};

// ============================================================
// BUTTON HANDLER
// ============================================================
void mode_menu_handle_button(button_event_t evt) {
  switch (evt) {
  case BTN_EVT_UP_PRESSED:
    if (g_ctx.menu_index > 0) {
      g_ctx.menu_index--;
      g_ctx.display_dirty = true;
      buzzer_click();
    }
    break;

  case BTN_EVT_DOWN_PRESSED:
    if (g_ctx.menu_index < MENU_ITEMS - 1) {
      g_ctx.menu_index++;
      g_ctx.display_dirty = true;
      buzzer_click();
    }
    break;

  case BTN_EVT_OK_SINGLE:
    // Select menu item
    switch (g_ctx.menu_index) {
    case 0:
      fsm_change_state(STATE_MODE_MECANUM);
      break;
    case 1:
      fsm_change_state(STATE_MODE_RC);
      break;
    case 2:
      fsm_change_state(STATE_MODE_VOICE);
      break;
    case 3:
      fsm_change_state(STATE_MODE_SETTINGS);
      break;
    }
    break;

  default:
    break;
  }
}

// ============================================================
// DRAW
// ============================================================
void mode_menu_draw(void) {
  ui_draw_header("MINI OS v1");

  // Draw menu items
  for (int i = 0; i < MENU_ITEMS; i++) {
    int y = 14 + i * 12;
    ui_draw_menu_item(y, s_menu_items[i], i == g_ctx.menu_index);
  }

  ui_draw_status_bar();
}
