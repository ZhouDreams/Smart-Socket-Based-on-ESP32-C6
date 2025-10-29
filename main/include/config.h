/*
    File: config.h
    Memo: 所有需要配置的参数
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "esp_netif.h"
#include "esp_wifi.h"

//----------FLAGS----------
extern int Air780EP_ONLINE_FLAG;
extern int WIFI_CONNECTED_FLAG;
extern int MQTT_WIFI_CONNECTED_FLAG;
extern int RELAY_STATUS_FLAG;
extern int AT_CMD_SENDING_FLAG;

//----------UART related----------
#define UART_4G_NUM UART_NUM_1
#define UART_4G_BAUD_RATE 115200
#define UART_4G_TX 10 //GPIO10
#define UART_4G_RX 11 //GPIO11
extern uart_config_t uart_config_4G;

#define UART_BL0942_NUM LP_UART_NUM_0
#define UART_BL0942_BAUD_RATE 9600 //BL0942
#define UART_BL0942_TX 5 //GPIO5
#define UART_BL0942_RX 4 //GPIO4
extern uart_config_t uart_config_BL0942;

#define BUF_SIZE 1024

//----------GPIO Related----------
#define GPIO0_PIN 8 //继电器GPIO0
#define GPIO1_PIN 9 //按钮GPIO1
#define GPIO_RELAY_NUM GPIO_NUM_0
#define GPIO_BUTTON_NUM GPIO_NUM_1
#define GPIO_WIFI_NUM GPIO_NUM_18
#define LED_ON 1
#define LED_OFF 0

//----------Interruption related----------

extern QueueHandle_t uart_4G_event_queue;  //UART1串口中断队列句柄
extern QueueHandle_t relay_event_queue; //继电器事件队列句柄

//----------Relay related----------
#define RELAY_ON 1
#define RELAY_OFF 0

typedef enum{
    FROM_BUTTON = 0,
    FROM_INTERNET = 1
} RELAY_CHANGE_SOURCE;  //继电器状态改变时的事件来源类型

//----------WIFI related----------


//----------MQTT related----------
#define MQTT_URI "mqtt://mqtt.jovisdreams.site"
#define MQTT_CLIENT_ID "Smart_Socket_WIFI"
#define MQTT_USERNAME "zhoudreams"
#define MQTT_PASSWD "sbzjx250"


#endif