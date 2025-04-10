/**
 * @file app_nvs.c
 * @brief source file for the applications use of non-volatile storage
 */

 #include <stdbool.h>
 #include <stdio.h>
 #include <string.h>
 
 #include "esp_log.h"
 #include "nvs_flash.h"

 #include "app_nvs.h"
 #include "wifi_app.h"

 // Tag for logging the monitor
static const char TAG[] = "nvs";

// NVS name space used for station mode credentials
const char app_nvs_sta_creds_namespace[] = "stacreds";

 /**
 * Saves station mode wifi credentials to NVS
 * @return ESP_OK if succesful
 */
esp_err_t app_nvs_save_sta_creds(void)
{
    nvs_handle handle;
    esp_err_t esp_err;
    ESP_LOGI(TAG, "app_nvs_save_sta_creds: Saving station mode credentials to flash");

    wifi_config_t *wifi_sta_config = wifi_app_get_wifi_config();

    if(wifi_sta_config)
    {
        esp_err = nvs_open(app_nvs_sta_creds_namespace, NVS_READWRITE, &handle);
        if(esp_err != ESP_OK)
        {
            printf("app_nvs_save_sta_creds: Error (%s) opening NVS handle!\n", esp_err_to_name(esp_err));
            return esp_err;
        }

        // Set SSID
        esp_err = nvs_set_blob(handle, "ssid", wifi_sta_config->sta.ssid, MAX_SSID_LENGTH);
        if(esp_err != ESP_OK)
        {
            printf("app_nvs_save_sta_creds: Error (%s) setting SSID to NVS!\n", esp_err_to_name(esp_err));
            return esp_err;
        }

        // Set Password
        esp_err = nvs_set_blob(handle, "password", wifi_sta_config->sta.password, MAX_PASSWORD_LENGTH);
        if(esp_err != ESP_OK)
        {
            printf("app_nvs_save_sta_creds: Error (%s) setting Password to NVS!\n", esp_err_to_name(esp_err));
            return esp_err;
        }

        //Commit creds to NVS
        esp_err = nvs_commit(handle);
        if(esp_err == ESP_OK)
        {
            printf("app_nvs_save_sta_creds: Error (%s) commiting credentials to NVS!\n", esp_err_to_name(esp_err));
            return esp_err;
        }
        nvs_close(handle);
        ESP_LOGI(TAG, "app_nvs_save_sta_creds: wrote wifi_sta_config: Station SSID %s and Password %s", wifi_sta_config->sta.ssid, wifi_sta_config->sta.password);
    }
    printf("app_nvs_save_sta_creds: returned ESP_OK\n");
    return ESP_OK;
}

/**
 * Loads the previously saved credentials from NVS
 * @return true if previously saved credentials were found.
 */
bool app_nvs_load_sta_creds(void)
{
    nvs_handle handle;
    esp_err_t esp_err;
    ESP_LOGI(TAG, "Loading WiFi Credentials from flash (NVS)");

    if(nvs_open(app_nvs_sta_creds_namespace, NVS_READONLY, &handle) == ESP_OK)
    {
        wifi_config_t *wifi_sta_config = wifi_app_get_wifi_config();

        if(wifi_sta_config == NULL)
        {
            wifi_sta_config = (wifi_config_t*)malloc(sizeof(wifi_config_t));
        }
        memset(wifi_sta_config, 0x00, sizeof(wifi_config_t));

        // Allocate buffer
        size_t wifi_config_size = sizeof(wifi_config_t);
        uint8_t *wifi_config_buff = (uint8_t*)malloc(sizeof(uint8_t) * wifi_config_size);
        memset(wifi_config_buff, 0x00, sizeof(wifi_config_size));

        // Load SSID
        wifi_config_size = sizeof(wifi_sta_config->sta.ssid);
        esp_err = nvs_get_blob(handle, "ssid", wifi_config_buff, &wifi_config_size);
        if(esp_err != ESP_OK)
        {
            free(wifi_config_buff);
            printf("app_nvs_load_sta_creds: (%s) no station SSID found in NVS\n", esp_err_to_name(esp_err));
            return false;
        }
        memcpy(wifi_sta_config->sta.ssid, wifi_config_buff, wifi_config_size);

        // Load SSID
        wifi_config_size = sizeof(wifi_sta_config->sta.password);
        esp_err = nvs_get_blob(handle, "password", wifi_config_buff, &wifi_config_size);
        if(esp_err != ESP_OK)
        {
            free(wifi_config_buff);
            printf("app_nvs_load_sta_creds: (%s) no station password found in NVS\n", esp_err_to_name(esp_err));
            return false;
        }
        memcpy(wifi_sta_config->sta.password, wifi_config_buff, wifi_config_size);

        free(wifi_config_buff);
        nvs_close(handle);

        printf("app_nvs_load_sta_creds: SSID: %s Password: %s\n", wifi_sta_config->sta.ssid, wifi_sta_config->sta.password);\
        return wifi_sta_config->sta.ssid[0] != '\0';
    }
    else
    {
        return false;
    }
    return false;
}

/**
 * Clears station mode credentials from NVS
 * @return ESP_OK if successful.
 */
esp_err_t app_nvs_clear_sta_creds(void)
{
    nvs_handle handle;
    esp_err_t esp_err;
    ESP_LOGI(TAG, "app_nvs_clear_sta_creds: Clearing Wifi station mode credentials from flash");

    esp_err = nvs_open(app_nvs_sta_creds_namespace, NVS_READWRITE, &handle);
    if(esp_err != ESP_OK)
    {
        printf("app_nvs_clear_sta_creds: Error (%s) opening NVS handle \n", esp_err_to_name(esp_err));
        return esp_err;
    }

    // Erase credentials
    esp_err = nvs_erase_all(handle);
    if(esp_err != ESP_OK)
    {
        printf("app_nvs_clear_sta_creds: Error (%s) erasing station mode credentials!\n", esp_err_to_name(esp_err));
        return esp_err;
    }

    // Commit clearing credentials from NVS
    esp_err = nvs_commit(handle);
    if(esp_err != ESP_OK)
    {
        printf("app_nvs_clear_sta_creds: Error (%s) NVS commit!\n", esp_err_to_name(esp_err));
        return esp_err;
    }
    nvs_close(handle);
    printf("app_nvs_clear_sta_creds: returned ESP_OK\n");
    return ESP_OK;
}