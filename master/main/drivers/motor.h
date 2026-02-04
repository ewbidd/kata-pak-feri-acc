/**
 * @file motor.h
 * @brief Motor control for Mecanum wheel robot
 */

#ifndef MOTOR_H
#define MOTOR_H

#include "types.h"

/**
 * @brief Initialize motor control (GPIO and PWM)
 */
void motor_init(void);

/**
 * @brief Stop all motors immediately
 */
void motor_stop_all(void);

/**
 * @brief Apply motor speeds
 * @param speeds Pointer to motor speeds struct
 */
void motor_apply_speeds(const motor_speeds_t *speeds);

/**
 * @brief Set calibration values
 * @param fl, fr, bl, br Calibration multipliers (0-255)
 */
void motor_set_calibration(uint8_t fl, uint8_t fr, uint8_t bl, uint8_t br);

/**
 * @brief Test single motor
 * @param motor_id 0=FL, 1=FR, 2=BL, 3=BR
 * @param speed Speed -255 to 255
 */
void motor_test(uint8_t motor_id, int16_t speed);

#endif // MOTOR_H
