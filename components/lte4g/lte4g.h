/*
    File: lte4g.h
    Memo: 控制和4G模块的通信
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/
#pragma once

#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AT_RESPONSE_DELAY 300
#define UART_4G_NUM UART_NUM_1
#define UART_4G_BAUD_RATE 115200
#define UART_4G_TX 10 //GPIO10
#define UART_4G_RX 11 //GPIO11
#define LTE4G_EN_GPIO 12
#define BUF_SIZE 1024
#define LTE4G_ONLINE BIT1
#define LTE4G_OFFLINE BIT0
#define AT_WAIT_TICKS_NORMAL pdMS_TO_TICKS(1000)

//Air780EP AT INIT CMD
#define AT_CIMI "AT+CIMI\r\n"
#define AT_RESET "AT+RESET\r\n" //重启模块
#define AT_CPIN "AT+CPIN?\r\n" //查询SIM卡是否准备好
#define AT_CSQ "AT+CSQ\r\n" //查询信号强度
#define AT_CGATT "AT+CGATT?\r\n" //查询上网服务是否激活
#define AT_CIPSHUT "AT+CIPSHUT\r\n" //关闭移动场景
#define AT_CSTT "AT+CSTT\r\n" //配置数据网络
#define AT_CIICR "AT+CIICR\r\n" //激活数据网络
#define AT_CIFSR "AT+CIFSR\r\n" //查询数据网络是否激活成功

//Air780EP AT MQTT CMD
#define AT_MCONFIG "AT+MCONFIG=\"lte4g-25108143g\",\"\",\"\",0,0,\"25108143g/online\",\"0\"\r\n" //设置 MQTT 相关参数
#define AT_MIPSTART "AT+MIPSTART=\"broker.emqx.io\",1883\r\n" //建立 TCP 连接
#define AT_MCONNECT "AT+MCONNECT=1,10\r\n" //客户端向服务器请求MQTT会话连接
#define AT_MDISCONNECT "AT+MDISCONNECT\r\n" //关闭MQTT会话连接
#define AT_MQTTSTATU "AT+MQTTSTATU\r\n" //查询MQTT连接状态

void lte4g_module_init_task_start(int priority); //初始化4G模块

EventGroupHandle_t lte4g_get_online_event();

// void AIR780EP_LIVE_DAEMON(); //检测4G联网是否正常

char* lte4g_send_at_cmd(const char* cmd, const char* wait_str, const char* error_str, TickType_t wait_time_ticks); //发送AT指令并返回串口的回复内容，wait_str指的是收到该回复字符串就结束，如发送命令后期待“OK”

char* lte4g_send_at_no_print(const char* cmd, const char* wait_str, const char* error_str, TickType_t wait_time_ticks); //发送AT指令并返回串口的回复内容，但是不print

#ifdef __cplusplus
}
#endif