/**
 * @file ui_common.h
 * @brief Common UI drawing functions
 */

#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <stdbool.h>

// Display drawing primitives (implemented in display.c)
extern void display_set_pixel(int x, int y, bool on);
extern void display_draw_char(int x, int y, char c);
extern void display_draw_string(int x, int y, const char *str);
extern void display_fill_rect(int x, int y, int w, int h, bool on);
extern void display_draw_rect(int x, int y, int w, int h);

/**
 * @brief Draw menu header
 * @param title Header title string
 */
void ui_draw_header(const char *title);

/**
 * @brief Draw menu item
 * @param y Y position
 * @param text Item text
 * @param selected Whether item is selected
 */
void ui_draw_menu_item(int y, const char *text, bool selected);

/**
 * @brief Draw connection status bar
 */
void ui_draw_status_bar(void);

/**
 * @brief Draw progress bar
 * @param x, y, w, h Position and size
 * @param value Current value (0-255)
 */
void ui_draw_progress_bar(int x, int y, int w, int h, uint8_t value);

/**
 * @brief Draw movement indicator
 * @param movement Movement type
 */
void ui_draw_movement(int movement);

#endif // UI_COMMON_H
