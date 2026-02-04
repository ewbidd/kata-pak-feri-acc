/**
 * @file espnow_handler.c
 * @brief ESP-NOW receive handling for joystick and voice data
 */

#include "esp_log.h"
#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>


#include "config.h"
#include "espnow_handler.h"
#include "types.h"


static const char *TAG = "ESPNOW";

// ============================================================
// ESP-NOW RECEIVE CALLBACK
// ============================================================
static void on_data_recv(const esp_now_recv_info_t *recv_info,
                         const uint8_t *data, int len) {
  uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

  ESP_LOGD(TAG, "Received %d bytes from " MACSTR, len,
           MAC2STR(recv_info->src_addr));

  // Check for joystick data (13 bytes = sizeof(joystick_data_t))
  if (len == sizeof(joystick_data_t)) {
    memcpy((void *)&g_ctx.joystick, data, sizeof(joystick_data_t));
    g_ctx.last_joystick_time = now;

    if (!g_ctx.joystick_connected) {
      g_ctx.joystick_connected = true;
      g_ctx.display_dirty = true;
      ESP_LOGI(TAG, "Joystick connected");
    }

    ESP_LOGD(TAG, "Joystick: T=%d S=%d", g_ctx.joystick.throttle,
             g_ctx.joystick.steering);
    return;
  }

  // Check for voice command (1 or 2 bytes)
  if (len == 1 || len == 2) {
    g_ctx.voice_cmd = data[0];
    g_ctx.voice_speed = (len == 2) ? data[1] : VOICE_DEFAULT_SPEED;
    g_ctx.last_voice_time = now;

    if (!g_ctx.voice_connected) {
      g_ctx.voice_connected = true;
      g_ctx.display_dirty = true;
      ESP_LOGI(TAG, "Voice slave connected");
    }

    ESP_LOGI(TAG, "Voice CMD: %d, Speed: %d", g_ctx.voice_cmd,
             g_ctx.voice_speed);
    return;
  }

  ESP_LOGW(TAG, "Unknown packet size: %d", len);
}

// ============================================================
// INITIALIZATION
// ============================================================
void espnow_handler_init(void) {
  // Initialize ESP-NOW
  esp_err_t ret = esp_now_init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "ESP-NOW init failed: %s", esp_err_to_name(ret));
    return;
  }

  // Register receive callback
  ret = esp_now_register_recv_cb(on_data_recv);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register recv callback: %s", esp_err_to_name(ret));
    return;
  }

  ESP_LOGI(TAG, "ESP-NOW handler initialized");
}
