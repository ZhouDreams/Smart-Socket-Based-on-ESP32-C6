/*
    File: mqtt.c
    Memo: MQTT协议和会话相关
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "config.h"

#define TAG "mqtt.c"

void MQTT_4G_INST()
{
    ESP_LOGI(TAG, "Initializing MQTT...");

    uart_flush(UART_4G_NUM);

    // SEND_AT_CMD(AT_RESET);
    // vTaskDelay(pdMS_TO_TICKS(1000));

    // SEND_AT_CMD(AT_CPIN);
    // SEND_AT_CMD(AT_CGATT);
    // SEND_AT_CMD(AT_MCONFIG);
    // SEND_AT_CMD(AT_MIPSTART);
    // SEND_AT_CMD(AT_MCONNECT);
    // SEND_AT_CMD(AT_MSUB_RELAY);

    // RELAY_STATUS_UPDATE(RELAY_OFF); //上电后继电器初始为断开

    ESP_LOGI(TAG, "MQTT inst Complete!\n");

}

void MQTT_WIFI_INST()
{
    
}