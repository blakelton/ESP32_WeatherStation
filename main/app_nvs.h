/**
 * @file app_nvs.h
 * @brief Header file for the applications use of non-volatile storage
 */
#ifndef MAIN_APP_NVS_H_
#define MAIN_APP_NVS_H_

/**
 * Saves station mode wifi credentials to NVS
 * @return ESP_OK if succesful
 */
esp_err_t app_nvs_save_sta_creds(void);

/**
 * Loads the previously saved credentials from NVS
 * @return true if previously saved credentials were found.
 */
bool app_nvs_load_sta_creds(void);

/**
 * Clears station mode credentials from NVS
 * @return ESP_OK if successful.
 */
esp_err_t app_nvs_clear_sta_creds(void);

#endif