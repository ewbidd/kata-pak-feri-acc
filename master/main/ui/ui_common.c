/**
 * @file ui_common.c
 * @brief Common UI drawing functions implementation
 */

#include "ui_common.h"
#include "config.h"
#include "types.h"
#include <stdio.h>


// ============================================================
// DRAW HEADER
// ============================================================
void ui_draw_header(const char *title) {
  // Draw header line
  display_fill_rect(0, 0, OLED_WIDTH, 10, true);

  // Draw title (inverted)
  int x = (OLED_WIDTH - strlen(title) * 6) / 2;
  for (int i = 0; title[i]; i++) {
    // Draw inverted character
    for (int col = 0; col < 5; col++) {
      for (int row = 0; row < 7; row++) {
        display_set_pixel(x + col, 1 + row, false);
      }
    }
    display_draw_char(x, 1, title[i]);
    // Invert the character
    for (int col = 0; col < 6; col++) {
      for (int row = 0; row < 8; row++) {
        int px_x = x + col;
        int px_y = 1 + row;
        if (px_x < OLED_WIDTH && px_y < 10) {
          // Toggle pixel for inversion effect
        }
      }
    }
    x += 6;
  }
}

// ============================================================
// DRAW MENU ITEM
// ============================================================
void ui_draw_menu_item(int y, const char *text, bool selected) {
  if (selected) {
    display_fill_rect(0, y, OLED_WIDTH, 10, true);
    // Draw inverted text
    int x = 4;
    for (int i = 0; text[i] && x < OLED_WIDTH - 6; i++) {
      // We need to invert - draw white background already done
      // Just draw the char normally then invert
      display_draw_char(x, y + 1, text[i]);
      x += 6;
    }
    // Invert: clear the character pixels (since bg is white)
    // For simplicity, just draw "> " indicator
    display_draw_string(0, y + 1, ">");
    display_draw_string(8, y + 1, text);
  } else {
    display_draw_string(8, y + 1, text);
  }
}

// ============================================================
// DRAW STATUS BAR
// ============================================================
void ui_draw_status_bar(void) {
  int y = OLED_HEIGHT - 9;

  // Draw separator line
  for (int x = 0; x < OLED_WIDTH; x++) {
    display_set_pixel(x, y, true);
  }

  // Joystick status
  if (g_ctx.joystick_connected) {
    display_draw_string(2, y + 2, "JOY:OK");
  } else {
    display_draw_string(2, y + 2, "JOY:--");
  }

  // Voice status
  if (g_ctx.voice_connected) {
    display_draw_string(70, y + 2, "VOI:OK");
  } else {
    display_draw_string(70, y + 2, "VOI:--");
  }
}

// ============================================================
// DRAW PROGRESS BAR
// ============================================================
void ui_draw_progress_bar(int x, int y, int w, int h, uint8_t value) {
  // Draw border
  display_draw_rect(x, y, w, h);

  // Calculate fill width
  int fill_w = ((w - 4) * value) / 255;

  // Draw fill
  display_fill_rect(x + 2, y + 2, fill_w, h - 4, true);
}

// ============================================================
// DRAW MOVEMENT INDICATOR
// ============================================================
void ui_draw_movement(int movement) {
  const char *str = "STOP";

  switch ((movement_type_t)movement) {
  case MOVEMENT_STOP:
    str = "STOP";
    break;
  case MOVEMENT_FORWARD:
    str = "FORWARD";
    break;
  case MOVEMENT_BACKWARD:
    str = "BACKWARD";
    break;
  case MOVEMENT_STRAFE_LEFT:
    str = "STRAFE L";
    break;
  case MOVEMENT_STRAFE_RIGHT:
    str = "STRAFE R";
    break;
  case MOVEMENT_ROTATE_LEFT:
    str = "ROTATE L";
    break;
  case MOVEMENT_ROTATE_RIGHT:
    str = "ROTATE R";
    break;
  case MOVEMENT_TURN_LEFT:
    str = "TURN L";
    break;
  case MOVEMENT_TURN_RIGHT:
    str = "TURN R";
    break;
  case MOVEMENT_EMERGENCY:
    str = "EMERGENCY";
    break;
  }

  // Center the text
  int x = (OLED_WIDTH - strlen(str) * 6) / 2;
  display_draw_string(x, 30, str);
}
