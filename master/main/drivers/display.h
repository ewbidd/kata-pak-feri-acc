/**
 * @file display.h
 * @brief OLED SSD1306 display driver
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include "types.h"

/**
 * @brief Initialize OLED display
 */
void display_init(void);

/**
 * @brief Show splash screen
 */
void display_splash(void);

/**
 * @brief Update display based on current state
 */
void display_update(void);

/**
 * @brief Set display brightness
 * @param brightness 0-255
 */
void display_set_brightness(uint8_t brightness);

/**
 * @brief Clear display
 */
void display_clear(void);

#endif // DISPLAY_H
