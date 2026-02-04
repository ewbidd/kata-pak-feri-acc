/**
 * @file motor.c
 * @brief Motor control for Mecanum wheel robot
 */

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

#include "config.h"
#include "motor.h"

static const char *TAG = "MOTOR";

// Calibration values (0-255, default 255 = no reduction)
static uint8_t s_cal_fl = DEFAULT_MOTOR_CAL;
static uint8_t s_cal_fr = DEFAULT_MOTOR_CAL;
static uint8_t s_cal_bl = DEFAULT_MOTOR_CAL;
static uint8_t s_cal_br = DEFAULT_MOTOR_CAL;

// ============================================================
// SET SINGLE MOTOR
// ============================================================
static void set_motor(int ena_ch, int in1_pin, int in2_pin, int16_t speed,
                      uint8_t cal) {
  // Apply calibration
  int16_t calibrated_speed = (speed * cal) / 255;

  if (calibrated_speed > 0) {
    gpio_set_level(in1_pin, 1);
    gpio_set_level(in2_pin, 0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, ena_ch, calibrated_speed);
  } else if (calibrated_speed < 0) {
    gpio_set_level(in1_pin, 0);
    gpio_set_level(in2_pin, 1);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, ena_ch, -calibrated_speed);
  } else {
    gpio_set_level(in1_pin, 0);
    gpio_set_level(in2_pin, 0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, ena_ch, 0);
  }
  ledc_update_duty(LEDC_LOW_SPEED_MODE, ena_ch);
}

// ============================================================
// INITIALIZATION
// ============================================================
void motor_init(void) {
  // Configure direction pins as outputs
  gpio_config_t io_conf = {
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
      .pin_bit_mask = (1ULL << PIN_FL_IN1) | (1ULL << PIN_FL_IN2) |
                      (1ULL << PIN_FR_IN1) | (1ULL << PIN_FR_IN2) |
                      (1ULL << PIN_BL_IN1) | (1ULL << PIN_BL_IN2) |
                      (1ULL << PIN_BR_IN1) | (1ULL << PIN_BR_IN2),
  };
  gpio_config(&io_conf);

  // Configure LEDC timer for motors
  ledc_timer_config_t timer_conf = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = MOTOR_PWM_RES,
      .timer_num = MOTOR_PWM_TIMER,
      .freq_hz = MOTOR_PWM_FREQ,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

  // Configure LEDC channels
  ledc_channel_config_t ch_conf = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .timer_sel = MOTOR_PWM_TIMER,
      .duty = 0,
      .hpoint = 0,
      .intr_type = LEDC_INTR_DISABLE,
  };

  // FL motor
  ch_conf.channel = MOTOR_CH_FL;
  ch_conf.gpio_num = PIN_FL_ENA;
  ESP_ERROR_CHECK(ledc_channel_config(&ch_conf));

  // FR motor
  ch_conf.channel = MOTOR_CH_FR;
  ch_conf.gpio_num = PIN_FR_ENA;
  ESP_ERROR_CHECK(ledc_channel_config(&ch_conf));

  // BL motor
  ch_conf.channel = MOTOR_CH_BL;
  ch_conf.gpio_num = PIN_BL_ENA;
  ESP_ERROR_CHECK(ledc_channel_config(&ch_conf));

  // BR motor
  ch_conf.channel = MOTOR_CH_BR;
  ch_conf.gpio_num = PIN_BR_ENA;
  ESP_ERROR_CHECK(ledc_channel_config(&ch_conf));

  motor_stop_all();

  ESP_LOGI(TAG, "Motor control initialized");
}

// ============================================================
// STOP ALL MOTORS
// ============================================================
void motor_stop_all(void) {
  set_motor(MOTOR_CH_FL, PIN_FL_IN1, PIN_FL_IN2, 0, 255);
  set_motor(MOTOR_CH_FR, PIN_FR_IN1, PIN_FR_IN2, 0, 255);
  set_motor(MOTOR_CH_BL, PIN_BL_IN1, PIN_BL_IN2, 0, 255);
  set_motor(MOTOR_CH_BR, PIN_BR_IN1, PIN_BR_IN2, 0, 255);
}

// ============================================================
// APPLY MOTOR SPEEDS
// ============================================================
void motor_apply_speeds(const motor_speeds_t *speeds) {
  set_motor(MOTOR_CH_FL, PIN_FL_IN1, PIN_FL_IN2, speeds->fl, s_cal_fl);
  set_motor(MOTOR_CH_FR, PIN_FR_IN1, PIN_FR_IN2, speeds->fr, s_cal_fr);
  set_motor(MOTOR_CH_BL, PIN_BL_IN1, PIN_BL_IN2, speeds->bl, s_cal_bl);
  set_motor(MOTOR_CH_BR, PIN_BR_IN1, PIN_BR_IN2, speeds->br, s_cal_br);
}

// ============================================================
// SET CALIBRATION
// ============================================================
void motor_set_calibration(uint8_t fl, uint8_t fr, uint8_t bl, uint8_t br) {
  s_cal_fl = fl;
  s_cal_fr = fr;
  s_cal_bl = bl;
  s_cal_br = br;
  ESP_LOGI(TAG, "Calibration set: FL=%d FR=%d BL=%d BR=%d", fl, fr, bl, br);
}

// ============================================================
// TEST SINGLE MOTOR
// ============================================================
void motor_test(uint8_t motor_id, int16_t speed) {
  motor_stop_all();

  switch (motor_id) {
  case 0:
    set_motor(MOTOR_CH_FL, PIN_FL_IN1, PIN_FL_IN2, speed, 255);
    ESP_LOGI(TAG, "Testing FL motor, speed=%d", speed);
    break;
  case 1:
    set_motor(MOTOR_CH_FR, PIN_FR_IN1, PIN_FR_IN2, speed, 255);
    ESP_LOGI(TAG, "Testing FR motor, speed=%d", speed);
    break;
  case 2:
    set_motor(MOTOR_CH_BL, PIN_BL_IN1, PIN_BL_IN2, speed, 255);
    ESP_LOGI(TAG, "Testing BL motor, speed=%d", speed);
    break;
  case 3:
    set_motor(MOTOR_CH_BR, PIN_BR_IN1, PIN_BR_IN2, speed, 255);
    ESP_LOGI(TAG, "Testing BR motor, speed=%d", speed);
    break;
  }
}
