/**
 * @file main.c
 * @brief Mini OS v1 - Entry point and task creation
 *
 * Production-ready ESP32-S3 firmware for Mecanum robot master
 * with FSM architecture and event-driven design.
 */

#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs_flash.h"


#include "buttons.h"
#include "buzzer.h"
#include "config.h"
#include "display.h"
#include "espnow_handler.h"
#include "fsm.h"
#include "motor.h"
#include "nvs_storage.h"
#include "types.h"


static const char *TAG = "MAIN";

// Global system context
system_context_t g_ctx = {0};

// Task handles
static TaskHandle_t s_button_task_handle = NULL;
static TaskHandle_t s_display_task_handle = NULL;
static TaskHandle_t s_control_task_handle = NULL;

// ============================================================
// WIFI INITIALIZATION
// ============================================================
static void wifi_init(void) {
  ESP_LOGI(TAG, "Initializing WiFi...");

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE));

  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  ESP_LOGI(TAG, "WiFi initialized, MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// ============================================================
// BUTTON POLLING TASK
// ============================================================
static void button_task(void *arg) {
  ESP_LOGI(TAG, "Button task started");

  while (1) {
    button_event_t evt = buttons_poll();
    if (evt != BTN_EVT_NONE) {
      fsm_process_button(evt);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ============================================================
// DISPLAY UPDATE TASK
// ============================================================
static void display_task(void *arg) {
  ESP_LOGI(TAG, "Display task started");

  while (1) {
    if (g_ctx.display_dirty) {
      display_update();
      g_ctx.display_dirty = false;
    }
    vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_MS));
  }
}

// ============================================================
// CONTROL LOOP TASK
// ============================================================
static void control_task(void *arg) {
  ESP_LOGI(TAG, "Control task started");

  while (1) {
    // Update FSM (check timeouts)
    fsm_update();

    // Process control based on current state
    switch (g_ctx.current_state) {
    case STATE_MODE_MECANUM:
    case STATE_MODE_RC:
      if (g_ctx.joystick_connected) {
        fsm_process_joystick();
        motor_apply_speeds(&g_ctx.motor_speeds);
      }
      break;
    case STATE_MODE_VOICE:
      if (g_ctx.voice_connected) {
        fsm_process_voice();
        motor_apply_speeds(&g_ctx.motor_speeds);
      }
      break;
    default:
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(20)); // 50Hz control loop
  }
}

// ============================================================
// MAIN ENTRY POINT
// ============================================================
void app_main(void) {
  ESP_LOGI(TAG, "============================================");
  ESP_LOGI(TAG, "  MINI OS v1 - ESP32-S3 Master");
  ESP_LOGI(TAG, "  Mecanum Robot Control System");
  ESP_LOGI(TAG, "============================================");

  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);
  ESP_LOGI(TAG, "NVS initialized");

  // Load settings from NVS
  nvs_storage_load(&g_ctx.settings);
  ESP_LOGI(TAG, "Settings loaded: brightness=%d, volume=%d",
           g_ctx.settings.brightness, g_ctx.settings.volume);

  // Initialize hardware
  display_init();
  ESP_LOGI(TAG, "Display initialized");

  buttons_init();
  ESP_LOGI(TAG, "Buttons initialized");

  buzzer_init();
  ESP_LOGI(TAG, "Buzzer initialized");

  motor_init();
  ESP_LOGI(TAG, "Motor control initialized");

  // Initialize WiFi and ESP-NOW
  wifi_init();
  espnow_handler_init();
  ESP_LOGI(TAG, "ESP-NOW initialized");

  // Initialize FSM
  fsm_init();

  // Show splash screen
  display_splash();
  buzzer_startup();
  vTaskDelay(pdMS_TO_TICKS(1000));

  // Initial display update
  g_ctx.display_dirty = true;

  // Create tasks
  xTaskCreate(button_task, "button_task", 2048, NULL, 5, &s_button_task_handle);
  xTaskCreate(display_task, "display_task", 4096, NULL, 3,
              &s_display_task_handle);
  xTaskCreate(control_task, "control_task", 4096, NULL, 4,
              &s_control_task_handle);

  ESP_LOGI(TAG, "============================================");
  ESP_LOGI(TAG, "  System Ready");
  ESP_LOGI(TAG, "============================================");

  // Main task can idle or handle other duties
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
