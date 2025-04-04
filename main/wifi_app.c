/**
 * @file wifi_app.c
 * @brief WiFi application
 */
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/netdb.h"

#include "rgb_led.h"
#include "wifi_app.h"
#include "tasks_common.h"
#include "http_server.h"

// Tad used for ESP serial console messages
static const char TAG[] = "wifi_app";

// Queue handle used to maniupulate the main queue of events
static QueueHandle_t wifi_app_queue_handle;

// netif object for the Station and Access Point
esp_netif_t *esp_netif_sta = NULL;
esp_netif_t *esp_netif_ap = NULL;

/**
 * Wifi application event handler
 * @param arg data, aside from event data that is passed to the handler when it is called
 * @param event_base the base id of the event to register the handler for
 * @param event_id the id for the event to register the handler for
 * @param event_data event data
 */
static void wifi_app_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if(event_base == WIFI_EVENT)
    {
        switch(event_id)
        {
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_START");
                break;
            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_STOP");
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_STACONNECTED");
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_STADISCONNECTED");
                break;
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
                break;
            case WIFI_EVENT_STA_DISCONNECTED: 
                ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
                break;
        };
    }
    else if(event_base == IP_EVENT)
    {
        switch(event_id)
        {
            case IP_EVENT_STA_GOT_IP:
                ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
                //wifi_app_send_message(WIFI_APP_MSG_STA_CONNECTED_GOT_IP);
                break;
            case IP_EVENT_AP_STAIPASSIGNED:
                ESP_LOGI(TAG, "IP_EVENT_AP_STAIPASSIGNED");
                break;
        };
    }
}

/**
 * Initialized the wifi application event handler for wifi and tcp/ip events
 */
static void wifi_app_event_handler_init(void)
{
    // Event loop for the wifi driver
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //Event handler for the connection
    esp_event_handler_instance_t instance_wifi_event;
    esp_event_handler_instance_t instance_ip_event;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
                    WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, &instance_wifi_event));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
                        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_app_event_handler, NULL, &instance_ip_event));
}

/**
 * Initialized the TCP stack and WiFi configuration
 */
static void wifi_app_default_wifi_init(void)
{
    // Initialize the TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Default WiFi configuration
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    // Create the default event loop for the station
    esp_netif_sta = esp_netif_create_default_wifi_sta();
    //assert(esp_netif_sta);
    // Create the default event loop for the access point
    esp_netif_ap = esp_netif_create_default_wifi_ap();
    //assert(esp_netif_ap);
}

/**
 * Configures the WiFi access point settings and assigns the static IP to the SoftAP
 */

static void wifi_app_soft_ap_config(void)
{
    // SoftAP WiFi Access point configuration
    wifi_config_t ap_config = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .ssid_len = strlen(WIFI_AP_SSID),
            .password = WIFI_AP_PASSWORD,
            .channel = WIFI_AP_CHANNEL,
            .ssid_hidden = WIFI_AP_SSID_VISIBILITY,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .max_connection = WIFI_AP_MAX_CONNECTIONS,
            .beacon_interval = WIFI_AP_BECON_INTERVAL,
        },
    };
    // configure DHCP for the access point
    esp_netif_ip_info_t ip_info;
    memset(&ip_info, 0x00, sizeof(ip_info));

    esp_netif_dhcps_stop(esp_netif_ap); // This call is needed first
    inet_pton(AF_INET, WIFI_AP_ADDRESS_IP, &ip_info.ip); // Assign access points static IP, GW, and netmask
    inet_pton(AF_INET, WIFI_AP_GATEWAY, &ip_info.gw);
    inet_pton(AF_INET, WIFI_AP_NETMASK, &ip_info.netmask);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ip_info)); //Statically configures the network interface
    ESP_ERROR_CHECK(esp_netif_dhcps_start(esp_netif_ap)); // Start the AP DHCP Server for connecting devices (e.g. mobile phones, laptops, etc.)
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA)); // Set the WiFi mode to AP and Station mode
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config)); // Set the configuration for the access point
    ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_AP, WIFI_AP_BANDWIDTH)); // Set the bandwidth for the access point (default is 20 MHz)
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_AP_MAX_TX_POWER)); // Set the power save mode to none
}

/**
 * Main task for the WiFi application
 * @param pvParameters parameter which can be passed to the task
 */
static void wifi_app_task(void *pvParameters)
{
    wifi_app_queue_message_t msg;

    // Initialize the event handler
    wifi_app_event_handler_init();

    // Initialize the TCP/IP stack and WiFi configuration
    wifi_app_default_wifi_init();

    // SoftAP configuration
    wifi_app_soft_ap_config();

    // Start Wifi
    ESP_ERROR_CHECK(esp_wifi_start());

    // Send first event message
    wifi_app_send_message(WIFI_APP_MSG_START_HTTP_SERVER);

    for(;;)
    {
        // inifintate loop
        if(xQueueReceive(wifi_app_queue_handle, &msg, portMAX_DELAY))
        {
            switch(msg.message_id)
            {
                case WIFI_APP_MSG_START_HTTP_SERVER:
                    ESP_LOGI(TAG, "Starting HTTP Server...");
                    http_server_start();
                    rgb_led_http_server_started();
                    break;
                case WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER:
                    ESP_LOGI(TAG, "Connecting from ...");
                    break;
                case WIFI_APP_MSG_STA_CONNECTED_GOT_IP:
                    ESP_LOGI(TAG, "Connected to AP and got IP...");
                    rgb_led_wifi_connected();
                    break;
                default:
                    ESP_LOGW(TAG, "Unknown message ID: %d", msg.message_id);
                    break;
            }
        }
    }
}

BaseType_t wifi_app_send_message(wifi_app_message_e msgID)
{
    wifi_app_queue_message_t msg;
    msg.message_id = msgID;
    return xQueueSend(wifi_app_queue_handle, &msg, portMAX_DELAY);

}

/**
 * Starts the WiFi RTOS task
 */
void wifi_app_start(void)
{
    ESP_LOGI(TAG, "Starting WiFi Application...");

    // Start WiFi started LED
    rgb_led_wifi_app_started();

    // Disable default wifi logging to keep clean
    esp_log_level_set("wifi", ESP_LOG_NONE);

    // Create message queue using wifi app queue
    wifi_app_queue_handle = xQueueCreate(3, sizeof(wifi_app_queue_message_t));

    // Start the wifi application task
    xTaskCreatePinnedToCore(&wifi_app_task, "wifi_app_task", WIFI_APP_TASK_STACK_SIZE, 
                            NULL, WIFI_APP_TASK_PRIORITY, NULL, WIFI_TASK_CORE_ID);
}