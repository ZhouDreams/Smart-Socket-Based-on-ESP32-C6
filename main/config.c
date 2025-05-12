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

#define TAG "config.c"

uart_config_t uart_config_4G = {
    .baud_rate = UART_4G_BAUD_RATE,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
};

uart_config_t uart_config_BL0942 = {
    .baud_rate = UART_BL0942_BAUD_RATE,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = LP_UART_SCLK_LP_FAST,
};

