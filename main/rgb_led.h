/**
 * @brief RGB LED control
 * @file rgb_led.h
 */

#ifndef MAIN_RGB_LED_H_
#define MAIN_RGB_LED_H_

// RGP LED GPIO
#define RGB_LED_R GPIO_NUM_4
#define RGB_LED_G GPIO_NUM_5
#define RGB_LED_B GPIO_NUM_6

// RGB LED Color mix channels
#define RGB_LED_CHANNEL_NUM 3

// RGB LED Configuration
typedef struct {
    int channel;
    int gpio;
    int mode;
    int timer_index;
} ledc_info_t;

extern ledc_info_t ledc_ch[RGB_LED_CHANNEL_NUM];

/**
 * Color to indicate wifi application started
 */
void rgb_led_wifi_app_started(void);

/**
 * Color to indicate the HTTP server has started
 */
void rgb_led_http_server_started(void);

/**
 * Color to indicate that the ESP32 is connected to an access point
 */
void rgb_led_wifi_connected(void);


#endif /* MAIN_RGB_LED_H_ */