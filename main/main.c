/**
 * @file main.c
 * @brief Main file for the WiFi application
 */

#include "nvs_flash.h"
#include "wifi_app.h"
#include "dht22.h"

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }   
    ESP_ERROR_CHECK(ret);

    // Start WiFi
    wifi_app_start();

    // Start DHT22 Sensor Task
    DHT22_task_start();

}
