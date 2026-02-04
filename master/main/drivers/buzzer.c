/**
 * @file buzzer.c
 * @brief Buzzer sound functions implementation
 */

#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "buzzer.h"
#include "config.h"


static const char *TAG = "BUZZER";

static uint8_t s_volume = DEFAULT_VOLUME;

// ============================================================
// INITIALIZATION
// ============================================================
void buzzer_init(void) {
  // Configure LEDC timer for buzzer
  ledc_timer_config_t timer_conf = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution = LEDC_TIMER_8_BIT,
      .timer_num = BUZZER_PWM_TIMER,
      .freq_hz = 1000,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ledc_timer_config(&timer_conf);

  // Configure LEDC channel
  ledc_channel_config_t ch_conf = {
      .gpio_num = PIN_BUZZER,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = BUZZER_PWM_CH,
      .timer_sel = BUZZER_PWM_TIMER,
      .duty = 0,
      .hpoint = 0,
  };
  ledc_channel_config(&ch_conf);

  ESP_LOGI(TAG, "Buzzer initialized on GPIO %d", PIN_BUZZER);
}

// ============================================================
// SET VOLUME
// ============================================================
void buzzer_set_volume(uint8_t volume) {
  s_volume = volume > 100 ? 100 : volume;
}

// ============================================================
// PLAY TONE
// ============================================================
void buzzer_tone(uint16_t freq, uint16_t duration) {
  if (s_volume == 0)
    return;

  // Calculate duty based on volume (max 50% duty cycle)
  uint8_t duty = (127 * s_volume) / 100;

  // Set frequency
  ledc_set_freq(LEDC_LOW_SPEED_MODE, BUZZER_PWM_TIMER, freq);

  // Set duty
  ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_PWM_CH, duty);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_PWM_CH);

  // Wait for duration
  vTaskDelay(pdMS_TO_TICKS(duration));

  // Stop
  ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_PWM_CH, 0);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_PWM_CH);
}

// ============================================================
// SOUND EFFECTS
// ============================================================
void buzzer_startup(void) {
  buzzer_tone(1000, 100);
  vTaskDelay(pdMS_TO_TICKS(50));
  buzzer_tone(1500, 100);
  vTaskDelay(pdMS_TO_TICKS(50));
  buzzer_tone(2000, 150);
}

void buzzer_click(void) { buzzer_tone(1500, 30); }

void buzzer_double_click(void) {
  buzzer_tone(1800, 30);
  vTaskDelay(pdMS_TO_TICKS(30));
  buzzer_tone(2200, 30);
}

void buzzer_error(void) {
  buzzer_tone(400, 100);
  vTaskDelay(pdMS_TO_TICKS(50));
  buzzer_tone(300, 150);
}
