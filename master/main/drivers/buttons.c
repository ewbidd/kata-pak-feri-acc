/**
 * @file buttons.c
 * @brief Button handling with debounce and event detection
 */

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "buttons.h"
#include "config.h"


static const char *TAG = "BUTTONS";

// Button state tracking
typedef struct {
  bool last_state;
  bool current_state;
  uint32_t last_press_time;
  uint32_t press_start_time;
  bool waiting_for_double;
  bool long_press_fired;
} button_state_t;

static button_state_t s_btn_up = {0};
static button_state_t s_btn_down = {0};
static button_state_t s_btn_ok = {0};

// ============================================================
// INITIALIZATION
// ============================================================
void buttons_init(void) {
  gpio_config_t io_conf = {
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
      .pin_bit_mask =
          (1ULL << PIN_BTN_UP) | (1ULL << PIN_BTN_DOWN) | (1ULL << PIN_BTN_OK),
  };
  gpio_config(&io_conf);

  ESP_LOGI(TAG, "Buttons initialized (UP=%d, DOWN=%d, OK=%d)", PIN_BTN_UP,
           PIN_BTN_DOWN, PIN_BTN_OK);
}

// ============================================================
// READ BUTTON (active LOW)
// ============================================================
static bool read_button(int pin) { return gpio_get_level(pin) == 0; }

// ============================================================
// POLL BUTTONS
// ============================================================
button_event_t buttons_poll(void) {
  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
  button_event_t result = BTN_EVT_NONE;

  // Read current state
  bool up_pressed = read_button(PIN_BTN_UP);
  bool down_pressed = read_button(PIN_BTN_DOWN);
  bool ok_pressed = read_button(PIN_BTN_OK);

  // ========== UP BUTTON ==========
  if (up_pressed && !s_btn_up.current_state) {
    // Just pressed
    if (now - s_btn_up.last_press_time > DEBOUNCE_MS) {
      s_btn_up.press_start_time = now;
      result = BTN_EVT_UP_PRESSED;
    }
  }
  s_btn_up.last_state = s_btn_up.current_state;
  s_btn_up.current_state = up_pressed;
  if (up_pressed)
    s_btn_up.last_press_time = now;

  // ========== DOWN BUTTON ==========
  if (down_pressed && !s_btn_down.current_state) {
    if (now - s_btn_down.last_press_time > DEBOUNCE_MS) {
      s_btn_down.press_start_time = now;
      result = BTN_EVT_DOWN_PRESSED;
    }
  }
  s_btn_down.last_state = s_btn_down.current_state;
  s_btn_down.current_state = down_pressed;
  if (down_pressed)
    s_btn_down.last_press_time = now;

  // ========== OK BUTTON (with double-click and long-press) ==========
  if (ok_pressed && !s_btn_ok.current_state) {
    // Just pressed
    if (now - s_btn_ok.last_press_time > DEBOUNCE_MS) {
      s_btn_ok.press_start_time = now;
      s_btn_ok.long_press_fired = false;
    }
  } else if (ok_pressed && s_btn_ok.current_state) {
    // Still held
    if (!s_btn_ok.long_press_fired &&
        (now - s_btn_ok.press_start_time > LONG_PRESS_MS)) {
      result = BTN_EVT_OK_LONG;
      s_btn_ok.long_press_fired = true;
      s_btn_ok.waiting_for_double = false;
    }
  } else if (!ok_pressed && s_btn_ok.current_state) {
    // Just released
    if (!s_btn_ok.long_press_fired) {
      if (s_btn_ok.waiting_for_double) {
        // Second click within window
        result = BTN_EVT_OK_DOUBLE;
        s_btn_ok.waiting_for_double = false;
      } else {
        // Start waiting for double click
        s_btn_ok.waiting_for_double = true;
      }
    }
  }

  // Check for single click timeout
  if (s_btn_ok.waiting_for_double && !ok_pressed) {
    if (now - s_btn_ok.last_press_time > DOUBLE_CLICK_MS) {
      result = BTN_EVT_OK_SINGLE;
      s_btn_ok.waiting_for_double = false;
    }
  }

  s_btn_ok.last_state = s_btn_ok.current_state;
  s_btn_ok.current_state = ok_pressed;
  if (ok_pressed)
    s_btn_ok.last_press_time = now;

  return result;
}
