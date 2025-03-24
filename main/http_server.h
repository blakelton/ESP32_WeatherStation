#ifndef MAIN_HTTP_SERVER_H_
#define MAIN_HTTP_SERVER_H_

/**
 * @file http_server.h
 * @brief HTTP server
 */

 typedef enum http_server_message
 {
    HTTP_MSG_WIFI_CONNECT_INIT = 0,
    HTTP_MSG_WIFI_CONNECT_SUCCESS,
    HTTP_MSG_WIFI_CONNECT_FAILED,
    HTTP_MSG_WIFI_DISCONNECTED,
    HTTP_MSG_OTA_UPDATE_INITIALIZED,
    HTTP_MSG_OTA_UPDATE_SUCCESSFUL,
    HTTP_MSG_OTA_UPDATE_FAILED,
 } http_server_message_e;

 /**
  * Structure for the message queue
  */
 typedef struct http_server_queue_message
 {
    http_server_message_e msgID;
 } http_server_queue_message_t;

 /**
  * Sends a message to the queue
  * @param msgID the message ID from the http_server_message_e enum
  * @return pdTRUE if the message was successfully sent, otherwise pdFALSE
  * @note Expand this function for new requirements
  */
 BaseType_t http_server_monitor_send_message(http_server_message_e msgID);

 /**
  * Starts the http server task
  */
 void http_server_start(void);

 /**
  * Stops the http server task
  */
void http_server_stop(void);

#endif /* MAIN_HTTP_SERVER_H_ */