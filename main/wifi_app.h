/**
 * @file wifi_app.h
 * @brief WiFi application
 */
#ifndef MAIN_WIFI_APP_H_
#define MAIN_WIFI_APP_H_

#include "esp_netif.h"
#include "esp_wifi_types.h"
#include "freertos/FreeRTOS.h"
#include "portmacro.h"

// Wifi Application Settings
#define WIFI_AP_SSID "Weather Station"
#define WIFI_AP_PASSWORD "password"
#define WIFI_AP_CHANNEL 1
#define WIFI_AP_SSID_VISIBILITY 0
#define WIFI_AP_MAX_CONNECTIONS 5
#define WIFI_AP_BECON_INTERVAL 100 // 100ms
#define WIFI_AP_ADDRESS_IP "192.168.0.1" // IP address of the AP
#define WIFI_AP_GATEWAY "192.168.0.1" // Gateway of the AP (same as IP address)
#define WIFI_AP_NETMASK "255.255.255.0" // Netmask of the AP
#define WIFI_AP_BANDWIDTH WIFI_BW_HT20 // AP Bandwidth 20 MHz 72 Mbps
#define WIFI_AP_MAX_TX_POWER WIFI_PS_NONE // Maximum TX power of the AP
#define MAX_SSID_LENGTH 32               // IEEE standard length
#define MAX_PASSWORD_LENGTH 64           // IEEE standard length
#define MAX_CONNECTION_RETRIES 5         // Maximum number of connection retries

// netif object for the Station and Access Point
extern esp_netif_t *esp_netif_sta;
extern esp_netif_t *esp_netif_ap;

/**
 * Message IDs for the WiFi application task
 * @note Expand this based on application requirements
 */
typedef enum wifi_app_message
{
    WIFI_APP_MSG_START_HTTP_SERVER = 0,
    WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER,
    WIFI_APP_MSG_STA_CONNECTED_GOT_IP,
    WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT,
    WIFI_APP_MSG_STA_DISCONNECTED,
} wifi_app_message_e;

/**
 * Structure for the message queue
 * @note Expand this based on your application requirements e.g. add more types and parameters as needed
 */
typedef struct wifi_app_queue_message
{
    wifi_app_message_e message_id;
} wifi_app_queue_message_t;

/**
 * Sends a message to the queue
 * @param msgID message ID from the wifi_message_e enum
 * @return pdTrue if an item was successfully posted, otherwise pdFALSE
 * @note Expand the parameter list based on your requirements e.g. same as expanding the wifi_message_t.
 */
BaseType_t wifi_app_send_message(wifi_app_message_e msgID);

/**
 * Starts the WiFi RTOS task
 */
void wifi_app_start(void);

/**
 * get the wifi configuration
 */
wifi_config_t* wifi_app_get_wifi_config(void);

#endif /* MAIN_WIFI_APP_H_ */