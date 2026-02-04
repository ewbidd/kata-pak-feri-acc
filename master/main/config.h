/**
 * @file config.h
 * @brief Hardware configuration and constants for Mini OS v1
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// PIN DEFINITIONS - ESP32-S3
// ============================================================

// OLED Display (I2C)
#define PIN_OLED_SDA 9
#define PIN_OLED_SCL 10
#define OLED_I2C_NUM I2C_NUM_0
#define OLED_I2C_FREQ 400000
#define OLED_ADDRESS 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// Buzzer
#define PIN_BUZZER 11
#define BUZZER_PWM_CH LEDC_CHANNEL_4
#define BUZZER_PWM_TIMER LEDC_TIMER_1

// Buttons (active LOW with internal pull-up)
#define PIN_BTN_UP 12
#define PIN_BTN_DOWN 13
#define PIN_BTN_OK 38

// LED Status
#define PIN_LED_STATUS 48

// ============================================================
// MOTOR PIN DEFINITIONS
// ============================================================

// Motor Front Left (FL)
#define PIN_FL_ENA 4
#define PIN_FL_IN1 14
#define PIN_FL_IN2 21

// Motor Front Right (FR)
#define PIN_FR_ENA 5
#define PIN_FR_IN1 16
#define PIN_FR_IN2 17

// Motor Back Left (BL)
#define PIN_BL_ENA 18
#define PIN_BL_IN1 19
#define PIN_BL_IN2 20

// Motor Back Right (BR)
#define PIN_BR_ENA 7
#define PIN_BR_IN1 15
#define PIN_BR_IN2 8

// Motor PWM Configuration
#define MOTOR_PWM_FREQ 20000
#define MOTOR_PWM_RES LEDC_TIMER_8_BIT
#define MOTOR_PWM_TIMER LEDC_TIMER_0

#define MOTOR_CH_FL LEDC_CHANNEL_0
#define MOTOR_CH_FR LEDC_CHANNEL_1
#define MOTOR_CH_BL LEDC_CHANNEL_2
#define MOTOR_CH_BR LEDC_CHANNEL_3

// ============================================================
// TIMING CONSTANTS
// ============================================================
#define DEBOUNCE_MS 50
#define LONG_PRESS_MS 1000
#define DOUBLE_CLICK_MS 400
#define CONNECTION_TIMEOUT_MS 500
#define DISPLAY_UPDATE_MS 100

// ============================================================
// CONTROL PARAMETERS
// ============================================================
#define DEADZONE 25
#define DIAGONAL_RATIO 0.6f
#define MAX_SPEED 255

// ============================================================
// VOICE COMMAND DEFINITIONS
// ============================================================
#define VOICE_CMD_STOP 0x00
#define VOICE_CMD_FORWARD 0x01
#define VOICE_CMD_BACKWARD 0x02
#define VOICE_CMD_LEFT 0x03
#define VOICE_CMD_RIGHT 0x04
#define VOICE_DEFAULT_SPEED 150

// ============================================================
// ESP-NOW CONFIGURATION
// ============================================================
#define WIFI_CHANNEL 1

// ============================================================
// NVS KEYS
// ============================================================
#define NVS_NAMESPACE "mini_os"
#define NVS_KEY_BRIGHTNESS "brightness"
#define NVS_KEY_VOLUME "volume"
#define NVS_KEY_MOTOR_CAL_FL "cal_fl"
#define NVS_KEY_MOTOR_CAL_FR "cal_fr"
#define NVS_KEY_MOTOR_CAL_BL "cal_bl"
#define NVS_KEY_MOTOR_CAL_BR "cal_br"

// Default values
#define DEFAULT_BRIGHTNESS 255
#define DEFAULT_VOLUME 80
#define DEFAULT_MOTOR_CAL 255

#endif // CONFIG_H
