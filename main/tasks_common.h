/**
 * @file tasks_common.h
 * @brief Common task definitions
 */

#ifndef MAIN_TASKS_COMMON_H_
#define MAIN_TASKS_COMMON_H_

// WiFi Application Task
#define WIFI_APP_TASK_STACK_SIZE 4096
#define WIFI_APP_TASK_PRIORITY 5
#define WIFI_TASK_CORE_ID 0

//http server task
#define HTTP_SERVER_TASK_STACK_SIZE 8192
#define HTTP_SERVER_TASK_PRIORITY 4
#define HTTP_SERVER_TASK_CORE_ID 0

//http server monitor task
#define HTTP_SERVER_MONITOR_TASK_STACK_SIZE 4096
#define HTTP_SERVER_MONITOR_TASK_PRIORITY 3
#define HTTP_SERVER_MONITOR_TASK_CORE_ID 0

#endif /* MAIN_TASKS_COMMON_H_ */