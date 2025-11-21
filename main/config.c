/*
    File: config.c
    Memo: 所有跟配置外设有关的函数和变量，所有中断和队列的配置，所有自定义的配置结构体
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "config.h"

static const char* TAG = "config";

int Air780EP_ONLINE_FLAG = 0;
int WIFI_CONNECTED_FLAG = 0;
int MQTT_WIFI_CONNECTED_FLAG = 0;





