/**
 * @file buzzer.h
 * @brief Buzzer sound functions
 */

#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

/**
 * @brief Initialize buzzer
 */
void buzzer_init(void);

/**
 * @brief Play a tone
 * @param freq Frequency in Hz
 * @param duration Duration in ms
 */
void buzzer_tone(uint16_t freq, uint16_t duration);

/**
 * @brief Play startup sound
 */
void buzzer_startup(void);

/**
 * @brief Play click sound
 */
void buzzer_click(void);

/**
 * @brief Play double-click sound
 */
void buzzer_double_click(void);

/**
 * @brief Play error sound
 */
void buzzer_error(void);

/**
 * @brief Set volume (0-100)
 */
void buzzer_set_volume(uint8_t volume);

#endif // BUZZER_H
