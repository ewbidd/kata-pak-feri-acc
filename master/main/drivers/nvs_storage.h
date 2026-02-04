/**
 * @file nvs_storage.h
 * @brief NVS (Non-Volatile Storage) for settings persistence
 */

#ifndef NVS_STORAGE_H
#define NVS_STORAGE_H

#include "types.h"

/**
 * @brief Load settings from NVS
 * @param settings Pointer to settings struct to fill
 */
void nvs_storage_load(settings_data_t *settings);

/**
 * @brief Save settings to NVS
 * @param settings Pointer to settings struct to save
 */
void nvs_storage_save(const settings_data_t *settings);

#endif // NVS_STORAGE_H
