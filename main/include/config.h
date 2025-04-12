/*
    File: config.h
    Memo: 所有跟配置外设有关的函数和变量，所有自定义的配置结构体
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "wifi.h"
#include "esp_netif.h"
#include "esp_wifi.h"

//UART related
#define UART_4G_NUM UART_NUM_1
#define UART_4G_BAUD_RATE 115200
#define UART_4G_TX 6 //GPIO6
#define UART_4G_RX 7 //GPIO7

#define UART_BL0942_NUM LP_UART_NUM_0
#define UART_BL0942_BAUD_RATE 9600 //BL0942
#define UART_BL0942_TX 5 //GPIO5
#define UART_BL0942_RX 4 //GPIO4

#define BUF_SIZE 1024

void UART_4G_INST(); //4G UART初始化
void UART_BL0942_INST(); //BL0942 UART初始化

//GPIO Related
#define GPIO2_PIN 5 //继电器
#define GPIO3_PIN 6 //按钮

void GPIO2_INST(); //继电器状态初始化
void GPIO3_INST(); //按钮初始化

//Interruption related
extern QueueHandle_t uart0_event_queue; //UART0串口中断队列句柄
extern QueueHandle_t uart1_event_queue; //UART1串口中断队列句柄
extern QueueHandle_t relay_event_queue; //继电器控制队列句柄

void BUTTON_ISR_HANDLER(void* arg); //按钮GPIO中断服务函数

//Relay related
#define RELAY_ON 0
#define RELAY_OFF 1

//WIFI related


void WIFI_INIT();

//MQTT related
#define AT_MCONFIG "AT+MCONFIG=\"socket-0\",\"zhoudreams\",\"sbzjx250\"\r\n" //设置 MQTT 相关参数
#define AT_MIPSTART "AT+MIPSTART=\"mqtt.jovisdreams.site\",1883\r\n" //建立 TCP 连接
#define AT_MCONNECT "AT+MCONNECT=0,300\r\n" //客户端向服务器请求会话连接
#define AT_MPUB "AT+MPUB=\"smartsocket\",0,0,\"{\\0A\\22Relay_Status\\22: 1,\\0A\\22Power\\22: 2349\\0A}\"\r\n" //发布消息
#define AT_MPUB_RELAY_ON "AT+MPUB=\"smartsocket/relay_status\",0,0,0\r\n"
#define AT_MPUB_RELAY_OFF "AT+MPUB=\"smartsocket/relay_status\",0,0,1\r\n"
#define AT_MSUB_RELAY "AT+MSUB=\"smartsocket/relay_status\",0\r\n"
#define AT_PING "AT+CIPPING=\"117.72.37.56\"\r\n"

typedef enum{
    FROM_BUTTON = 0,
    FROM_INTERNET = 1
} RELAY_CHANGE_SOURCE;  //继电器状态改变时的事件来源类型


#endif