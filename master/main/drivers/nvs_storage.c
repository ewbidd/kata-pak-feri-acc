/**
 * @file nvs_storage.c
 * @brief NVS (Non-Volatile Storage) for settings persistence
 */

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"


#include "config.h"
#include "nvs_storage.h"

static const char *TAG = "NVS";

// ============================================================
// LOAD SETTINGS
// ============================================================
void nvs_storage_load(settings_data_t *settings) {
  nvs_handle_t handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);

  if (err != ESP_OK) {
    ESP_LOGW(TAG, "NVS namespace not found, using defaults");
    settings->brightness = DEFAULT_BRIGHTNESS;
    settings->volume = DEFAULT_VOLUME;
    settings->motor_cal_fl = DEFAULT_MOTOR_CAL;
    settings->motor_cal_fr = DEFAULT_MOTOR_CAL;
    settings->motor_cal_bl = DEFAULT_MOTOR_CAL;
    settings->motor_cal_br = DEFAULT_MOTOR_CAL;
    return;
  }

  uint8_t val;

  // Brightness
  if (nvs_get_u8(handle, NVS_KEY_BRIGHTNESS, &val) == ESP_OK) {
    settings->brightness = val;
  } else {
    settings->brightness = DEFAULT_BRIGHTNESS;
  }

  // Volume
  if (nvs_get_u8(handle, NVS_KEY_VOLUME, &val) == ESP_OK) {
    settings->volume = val;
  } else {
    settings->volume = DEFAULT_VOLUME;
  }

  // Motor calibration
  if (nvs_get_u8(handle, NVS_KEY_MOTOR_CAL_FL, &val) == ESP_OK) {
    settings->motor_cal_fl = val;
  } else {
    settings->motor_cal_fl = DEFAULT_MOTOR_CAL;
  }

  if (nvs_get_u8(handle, NVS_KEY_MOTOR_CAL_FR, &val) == ESP_OK) {
    settings->motor_cal_fr = val;
  } else {
    settings->motor_cal_fr = DEFAULT_MOTOR_CAL;
  }

  if (nvs_get_u8(handle, NVS_KEY_MOTOR_CAL_BL, &val) == ESP_OK) {
    settings->motor_cal_bl = val;
  } else {
    settings->motor_cal_bl = DEFAULT_MOTOR_CAL;
  }

  if (nvs_get_u8(handle, NVS_KEY_MOTOR_CAL_BR, &val) == ESP_OK) {
    settings->motor_cal_br = val;
  } else {
    settings->motor_cal_br = DEFAULT_MOTOR_CAL;
  }

  nvs_close(handle);

  ESP_LOGI(TAG, "Settings loaded from NVS");
}

// ============================================================
// SAVE SETTINGS
// ============================================================
void nvs_storage_save(const settings_data_t *settings) {
  nvs_handle_t handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS for writing");
    return;
  }

  nvs_set_u8(handle, NVS_KEY_BRIGHTNESS, settings->brightness);
  nvs_set_u8(handle, NVS_KEY_VOLUME, settings->volume);
  nvs_set_u8(handle, NVS_KEY_MOTOR_CAL_FL, settings->motor_cal_fl);
  nvs_set_u8(handle, NVS_KEY_MOTOR_CAL_FR, settings->motor_cal_fr);
  nvs_set_u8(handle, NVS_KEY_MOTOR_CAL_BL, settings->motor_cal_bl);
  nvs_set_u8(handle, NVS_KEY_MOTOR_CAL_BR, settings->motor_cal_br);

  nvs_commit(handle);
  nvs_close(handle);

  ESP_LOGI(TAG, "Settings saved to NVS");
}
