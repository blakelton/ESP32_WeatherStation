/**
 * @file http_server.c
 * @brief HTTP Server
 */

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_wifi.h"
#include "esp_timer.h"
#include "sys/param.h"

#include "dht22.h"
#include "cJSON.h"
#include "http_server.h"
#include "tasks_common.h"
#include "wifi_app.h"

// Tag used for ESP serial console messages
static const char *TAG = "http_server";

// Wifi connect status
static int g_wifi_connect_status = NONE;

// Firmware update status
static int g_fw_update_status = OTA_UPDATE_PENDING;

// HTTP Server Task handle
static httpd_handle_t http_server_handle = NULL;

// HTTP Server Monitor Task Handle
static TaskHandle_t task_http_server_monitor = NULL;

// Queue handle used to mainipulate the main queue of events
static QueueHandle_t http_server_monitor_queue_handle = NULL;

/**
 * ESP32 timer configuration passed to esp_timer_create.
 */
const esp_timer_create_args_t fw_update_reset_args = {
    .callback = &http_server_fw_update_reset_callback,
    .arg = NULL,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "fw_update_reset"  
};
esp_timer_handle_t fw_update_reset;

// Embedded files: JQuery, intdex.html, app.css, app.js and favicon.ico 
extern const uint8_t jquery_3_3_1_min_js_start[]    asm("_binary_jquery_3_3_1_min_js_start");
extern const uint8_t jquery_3_3_1_min_js_end[]      asm("_binary_jquery_3_3_1_min_js_end");
extern const uint8_t index_html_start[]             asm("_binary_index_html_start");
extern const uint8_t index_html_end[]               asm("_binary_index_html_end");
extern const uint8_t app_css_start[]                asm("_binary_app_css_start");
extern const uint8_t app_css_end[]                  asm("_binary_app_css_end");
extern const uint8_t app_js_start[]                 asm("_binary_app_js_start");
extern const uint8_t app_js_end[]                   asm("_binary_app_js_end");
extern const uint8_t favicon_ico_start[]            asm("_binary_favicon_ico_start");   
extern const uint8_t favicon_ico_end[]              asm("_binary_favicon_ico_end");

/**
 * @brief checks the global firmware update status (g_fw_update_status) and 
 * create the fw_update_status if true
 */
static void http_server_fw_update_reset_timer(void)
{
    if(g_fw_update_status == OTA_UPDATE_SUCCESSFUL)
    {
        ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW Updated succesfully, starting FW update reset timer");
        ESP_ERROR_CHECK(esp_timer_create(&fw_update_reset_args, &fw_update_reset));
        ESP_ERROR_CHECK(esp_timer_start_once(fw_update_reset, 8000000)); // 8 seconds
    }
    else
    {
        ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW Update failed, not starting FW update reset timer");
    }
}


/**
 * Http Server Monitor Task
 * @brief used to track events of the HTTP server
 * @param pvParameters which can be passed to the task
 */
static void http_server_monitor(void *parameter)
{
    http_server_queue_message_t msg;
    for(;;)
    {
        if(xQueueReceive(http_server_monitor_queue_handle, &msg, portMAX_DELAY))
        {
            switch(msg.msgID)
            {
                case HTTP_MSG_WIFI_CONNECT_INIT:
                    ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_INIT");
                    g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECTING;
                    break;
                case HTTP_MSG_WIFI_CONNECT_SUCCESS:
                    ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_SUCCESS");
                    g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_SUCCESS;
                    break;
                case HTTP_MSG_WIFI_CONNECT_FAILED:
                    ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_FAILED");
                    g_wifi_connect_status = HTTP_WIFI_STATUS_CONNECT_FAILED;
                    break;
                case HTTP_MSG_WIFI_USER_DISCONNECT:
                    ESP_LOGI(TAG, "HTTP_MSG_WIFI_USER_DISCONNECT");
                    g_wifi_connect_status = HTTP_WIFI_STATUS_DISCONNECTED;
                    break;
                case HTTP_MSG_WIFI_DISCONNECTED:
                    ESP_LOGI(TAG, "HTTP_MSG_WIFI_DISCONNECTED");
                    break;
                case HTTP_MSG_OTA_UPDATE_SUCCESSFUL: 
                    ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_SUCCESSFUL");
                    g_fw_update_status = OTA_UPDATE_SUCCESSFUL;
                    http_server_fw_update_reset_timer();
                    break;
                case HTTP_MSG_OTA_UPDATE_FAILED: 
                    ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_FAILED");
                    g_fw_update_status = OTA_UPDATE_FAILED;
                    break;
                default:
                    ESP_LOGW(TAG, "Unknown message ID: %d", msg.msgID);
                    break;
            }
        }
    }
}

/**
 * jQuery get handler is requested when accessing the index.html page
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK on success
 */
static esp_err_t http_server_jquery_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "jQuery requested");
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end - jquery_3_3_1_min_js_start);
    return ESP_OK;
}

/**
 * Sends the index.html page
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK on success
 */
static esp_err_t http_server_index_html_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "index requested");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

/**
 * app.css get handler is requested when accessing the web page
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK on success
 */
static esp_err_t http_server_app_css_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "app.css requested");
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)app_css_start, app_css_end - app_css_start);
    return ESP_OK;
}

/**
 * app.js get handler is requested when accessing the web page
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK on success
 */
static esp_err_t http_server_app_js_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "app.js requested");
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)app_js_start, app_js_end - app_js_start);
    return ESP_OK;
}

/**
 * send the favicon.ico file when accessing the webpage
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK on success
 */
static esp_err_t http_server_favicon_ico_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "favicon.ico requested");
    httpd_resp_set_type(req, "image/x-icon");
    httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_end - favicon_ico_start);
    return ESP_OK;
}

/**
 * Recieves the bin file via the webpage and handle the firmware update
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK on success, otherwise ESP_FAIL if timeout occurs and the update cannot be started
 */
esp_err_t http_server_ota_update_handler(httpd_req_t *req)
{
    esp_ota_handle_t ota_handle;
    
    char ota_buff[1024];
    int content_length = req->content_len;
    int content_received = 0;
    int recv_len = 0;
    bool is_req_body_started = false;
    bool flash_successful = false;

    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    do
    {
        if((recv_len = httpd_req_recv(req, ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0)
        {
            if(recv_len == HTTPD_SOCK_ERR_TIMEOUT)
            {
                ESP_LOGI(TAG, "http_server_OTA_update_handler: socket timeout");
                continue; // retry recieving if timeout occured
            }
            ESP_LOGI(TAG, "http_server_OTA_update_handler: OTA other error: %d", recv_len);
        }
        printf("http_server_OTA_update_handler: OTA RX: %d of %d (%d%%)\r", content_received, content_length, (int)((content_received * 100) / content_length));

        // Is this the first data we are receiving - If so we should pull header info we need
        if(!is_req_body_started)
        {
            is_req_body_started = true;
            // Get the location of the .bin file content (remove the web form data)
            char *body_start_p = strstr(ota_buff, "\r\n\r\n") + 4;
            int body_part_len = recv_len - (body_start_p - ota_buff);

            printf("http_server_OTA_update_handler: OTA file Size: %d\r\n", content_length);

            esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "http_server_OTA_update_handler: esp_ota_begin failed, error=%d", err);
                return ESP_FAIL;
            }
            else
            {
                printf("http_server_OTA_update_handler: Writing to partition subtype %d at offset 0x%lx\r\n",
                       update_partition->subtype, update_partition->address);
            }
            
            // Write the first part of the data
            esp_ota_write(ota_handle, body_start_p, body_part_len);
            content_received += body_part_len;
        }
        else
        {
            // Write the OTA data to the partition
            esp_ota_write(ota_handle, ota_buff, recv_len);
            content_received += recv_len;
        }
    } while (recv_len > 0 && content_received < content_length);
    
    if(esp_ota_end(ota_handle) == ESP_OK)
    {
        if(esp_ota_set_boot_partition(update_partition) == ESP_OK)
        {
            const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
            ESP_LOGI(TAG, "http_server_OTA_update_handler: Next boot partition subtype %d at offset 0x%lx",
                     boot_partition->subtype, boot_partition->address);
            flash_successful = true;
        }
        else
        {
            ESP_LOGI(TAG, "http_server_OTA_update_handler: FLASHING ERROR!");
        }
    }
    else
    {
        ESP_LOGI(TAG, "http_server_OTA_update_handler: FLASHING ERROR!");
    }
    // We wont update the global variables throughout the file, so send the message about the status
    if(flash_successful)
    {
        http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_SUCCESSFUL);
    }
    else
    {
        http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_FAILED);
    }
    return ESP_OK;
}

/**
 * OTA Handler responds with the firmware update status after the OTA update is started and responds
 * with the compile time/date when the page is first requested
 * @param req HTTP request for which the uri needs to be handled
 * @return ESP_OK on success
 */
esp_err_t http_server_ota_status_handler(httpd_req_t *req)
{
    char otaJSON[100];

    ESP_LOGI(TAG, "http_server_OTA_status_handler: OTA status requested");
    sprintf(otaJSON, "{\"ota_update_status\":%d,\"compile_time\":\"%s\", \"compile_date\":\"%s\"}", g_fw_update_status, __TIME__, __DATE__);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, otaJSON, strlen(otaJSON));
    
    return ESP_OK;
}

/**
 * DHT Sensor readings JSON handler responds with DHT22 sensor data
 * @parm req HTTP request for which the uri needs to be handled
 * @return ESP_OK
 */
 static esp_err_t http_server_get_dht_sensor_readings_json_handler(httpd_req_t *req)
 {
    ESP_LOGI(TAG, "/dhtSensor.json requested");
    char dhtSensorJSON[100];
    sprintf(dhtSensorJSON, "{\"temp\":\"%.1f\", \"humidity\":\"%.1f\"}", 
        getTemperatureFahrenheit(), getHumidity());
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, dhtSensorJSON, strlen(dhtSensorJSON));

    return ESP_OK;
 }

 /**
  * wifiConnect.json handler is invoked after the connect button is pressed
  * and handles receiving the SSID and password entered by the user
  * @param req HTTP request for which the uri needs to be handled.
  * @return ESP_OK
  */
 static esp_err_t http_server_wifi_connect_json_handler(httpd_req_t *req)
 {
     ESP_LOGI(TAG, "/wifiConnect.json requested");
 
     // Determine the length of the request body.
     int total_len = req->content_len;
     if(total_len <= 0) {
         ESP_LOGE(TAG, "Empty request body");
         return ESP_FAIL;
     }
     
     // Allocate a buffer for the body.
     char *buf = malloc(total_len + 1);
     if(buf == NULL) {
         ESP_LOGE(TAG, "Failed to allocate memory for request body");
         return ESP_ERR_NO_MEM;
     }
     
     // Read the body.
     int ret = httpd_req_recv(req, buf, total_len);
     if(ret <= 0) {
         ESP_LOGE(TAG, "Failed to read request body");
         free(buf);
         return ESP_FAIL;
     }
     buf[total_len] = '\0';  // Null-terminate the string.
     ESP_LOGI(TAG, "Received body: %s", buf);
     
     // Parse the JSON data.
     cJSON *json = cJSON_Parse(buf);
     free(buf);
     if(json == NULL) {
         ESP_LOGE(TAG, "Failed to parse JSON");
         return ESP_FAIL;
     }
     
     // Extract the SSID and password values.
     cJSON *ssid_json = cJSON_GetObjectItemCaseSensitive(json, "ssid");
     cJSON *password_json = cJSON_GetObjectItemCaseSensitive(json, "password");
     if(!cJSON_IsString(ssid_json) || !cJSON_IsString(password_json)) {
         ESP_LOGE(TAG, "Invalid JSON: missing or invalid ssid or password");
         cJSON_Delete(json);
         return ESP_FAIL;
     }
     
     const char *ssid_str = ssid_json->valuestring;
     const char *pwd_str = password_json->valuestring;
     
     // Update the WiFi configuration.
     wifi_config_t* wifi_config = wifi_app_get_wifi_config();
     memset(wifi_config, 0x00, sizeof(wifi_config_t));
     // Use strncpy to ensure no buffer overflow.
     strncpy((char*)wifi_config->sta.ssid, ssid_str, sizeof(wifi_config->sta.ssid) - 1);
     strncpy((char*)wifi_config->sta.password, pwd_str, sizeof(wifi_config->sta.password) - 1);
     
     // Signal the WiFi task to attempt a connection.
     wifi_app_send_message(WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER);
     
     cJSON_Delete(json);
     return ESP_OK;
 }
 

 /**
  * wifiConnectStatus Handler updates the connection status for the web page.
  * @param req HTTP request for which the uri needs to be handled.
  * @return ESP_OK
  */
 static esp_err_t http_server_wifi_connect_status_json_handler(httpd_req_t *req)
 {
    ESP_LOGI(TAG, "/wifiConnectStatus requested");

    char statusJSON[100];

    sprintf(statusJSON, "{\"wifi_connect_status\":%d}", g_wifi_connect_status);

    httpd_resp_set_type(req,"application/json");
    httpd_resp_send(req, statusJSON, strlen(statusJSON));

    return ESP_OK;
}

/**
  * wifiConnectInfo Handler updates the connection info for the web page.
  * @param req HTTP request for which the uri needs to be handled.
  * @return ESP_OK
  */
static esp_err_t http_server_get_wifi_connect_info_json_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "/wifiConnectInfo.json requested");

    char ipInfoJSON[200];
    memset(ipInfoJSON, 0, sizeof(ipInfoJSON));

    char ip[IP4ADDR_STRLEN_MAX];
    char netmask[IP4ADDR_STRLEN_MAX];
    char gw[IP4ADDR_STRLEN_MAX];

    if(g_wifi_connect_status == HTTP_WIFI_STATUS_CONNECT_SUCCESS)
    {
        wifi_ap_record_t wifi_data;
        ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&wifi_data));
        char *ssid = (char*)wifi_data.ssid;

        esp_netif_ip_info_t ip_info;
        //esp_netif_t *netif_sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif_sta, &ip_info));
        esp_ip4addr_ntoa(&ip_info.ip, ip, IP4ADDR_STRLEN_MAX);
        esp_ip4addr_ntoa(&ip_info.netmask, netmask, IP4ADDR_STRLEN_MAX);
        esp_ip4addr_ntoa(&ip_info.gw, gw, IP4ADDR_STRLEN_MAX);

        sprintf(ipInfoJSON, "{\"ip\":\"%s\",\"netmask\":\"%s\",\"gw\":\"%s\",\"ap\":\"%s\"}", ip, netmask, gw, ssid);
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, ipInfoJSON, strlen(ipInfoJSON));

    return ESP_OK;
}

/**
  * wifiDisconnect.json handler responds by sending a message to the wifi application to disconnect
  * @param req HTTP request for which the uri needs to be handled.
  * @return ESP_OK
  */
static esp_err_t http_server_wifi_disconnect_json_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "wifiDisconnect.json requested");

    wifi_app_send_message(WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT);

    return ESP_OK;
}

/**
 * sets up the default httpd server configuration
 * @return the http server instance handle if successful, NULL otherwise
 */
static httpd_handle_t http_server_configure(void)
{
    // Generate the default configuration
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    //create http server monitor task
    xTaskCreatePinnedToCore(&http_server_monitor, "http_server_monitor", HTTP_SERVER_MONITOR_TASK_STACK_SIZE, NULL, 
                            HTTP_SERVER_MONITOR_TASK_PRIORITY, &task_http_server_monitor, HTTP_SERVER_MONITOR_TASK_CORE_ID);

    //create the message queue
    http_server_monitor_queue_handle = xQueueCreate(3, sizeof(http_server_queue_message_t));

    // The core the http server will run on
    config.core_id = HTTP_SERVER_TASK_CORE_ID;

    // Adjust the default priority to 1 less than the wifi application task
    config.task_priority = WIFI_APP_TASK_PRIORITY - 1;

    // Bump up the stack size to 8192 bytes
    config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;

    // Increase URI handlers
    config.max_uri_handlers = 20;

    // Increase the timeout limits
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;

    ESP_LOGI(TAG, "HTTP Server configure: starting HTTP server on port: '%d' with task priority: '%d'",
              config.server_port, config.task_priority);

    // Start the httpd server
    if(httpd_start(&http_server_handle, &config) == ESP_OK)
    {
        ESP_LOGI(TAG, "http_server_congfigure: Registering URI handlers");

        // register query handler
        httpd_uri_t jquery_js = {
            .uri = "/jquery-3.3.1.min.js",
            .method = HTTP_GET,
            .handler = http_server_jquery_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handle, &jquery_js);

        // register index.html handler
        httpd_uri_t index_html = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = http_server_index_html_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handle, &index_html);

        // register app.css handler
        httpd_uri_t app_css = {
            .uri = "/app.css",
            .method = HTTP_GET,
            .handler = http_server_app_css_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handle, &app_css);

        httpd_uri_t app_js = {
            .uri = "/app.js",
            .method = HTTP_GET,
            .handler = http_server_app_js_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handle, &app_js);

        httpd_uri_t favicon_ico = {
            .uri = "/favicon.ico",
            .method = HTTP_GET,
            .handler = http_server_favicon_ico_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handle, &favicon_ico);

        // register OTA update handler
        httpd_uri_t OTA_update = {
            .uri = "/OTAupdate",
            .method = HTTP_POST,
            .handler = http_server_ota_update_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handle, &OTA_update);

        //register ota status handler
        httpd_uri_t OTA_status = {
            .uri = "/OTAstatus",
            .method = HTTP_POST,
            .handler = http_server_ota_status_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handle, &OTA_status);

        //register dht22Sensor.json handler
        httpd_uri_t dht_sensor_json = {
            .uri = "/dhtSensor.json",
            .method = HTTP_GET,
            .handler = http_server_get_dht_sensor_readings_json_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handle, &dht_sensor_json);

        //register wifiConnect.json handler
        httpd_uri_t wifi_connect_json = {
            .uri = "/wifiConnect.json",
            .method = HTTP_POST,
            .handler = http_server_wifi_connect_json_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handle, &wifi_connect_json);

        //register wificonnectstatus.json handler
        httpd_uri_t wifi_connect_status_json = {
            .uri = "/wifiConnectStatus",
            .method = HTTP_POST,
            .handler = http_server_wifi_connect_status_json_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handle, &wifi_connect_status_json);

        //register wifiConnectInfo.json handler
        httpd_uri_t wifi_connect_info_json = {
            .uri = "/wifiConnectInfo.json",
            .method = HTTP_GET,
            .handler = http_server_get_wifi_connect_info_json_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handle, &wifi_connect_info_json);

        //register wifiDisconnect.json handler
        httpd_uri_t wifi_disconnect_json = {
            .uri = "/wifiDisconnect.json",
            .method = HTTP_DELETE,
            .handler = http_server_wifi_disconnect_json_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(http_server_handle, &wifi_disconnect_json);

        return http_server_handle;
    }
    return NULL;
}

void http_server_start(void)
{
    if (http_server_handle == NULL)
    {
        http_server_handle = http_server_configure();
    }
}

void http_server_stop(void)
{
    if (http_server_handle)
    {
        httpd_stop(http_server_handle);
        ESP_LOGI(TAG, "HTTP Server stop: stopping HTTP server");
        http_server_handle = NULL;
    }
    if (task_http_server_monitor)
    {
        vTaskDelete(task_http_server_monitor);
        ESP_LOGI(TAG, "HTTP Server stop: stopping HTTP server monitor");
        task_http_server_monitor = NULL;
    }

}

BaseType_t http_server_monitor_send_message(http_server_message_e msgID)
{
    http_server_queue_message_t msg;
    msg.msgID = msgID;
    return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY);
}

void http_server_fw_update_reset_callback(void *arg)
{
    ESP_LOGI(TAG, "http_server_fw_update_reset_callback: Timer timed-out, restarting the device");
    esp_restart();
}