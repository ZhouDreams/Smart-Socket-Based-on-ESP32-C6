/*
    File: relay.h
    Memo: 操作继电器用的函数
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "mqtt.h"
#include "relay.h"
#include "config.h"

#define TAG "relay.c"

//继电器GPIO2初始化
void RELAY_GPIO2_INST()
{
    gpio_reset_pin(GPIO_RELAY_NUM);
    gpio_set_direction(GPIO_RELAY_NUM, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_RELAY_NUM, RELAY_OFF);
    ESP_LOGI(TAG, "Relay has been installed.");
}

//设置继电器开关
void RELAY_SET_LEVEL(int level)
{
    gpio_set_level(GPIO_RELAY_NUM, level);
}

//继电器任务
void RELAY_TASK()
{
    ESP_LOGI(TAG, "RELAY_TASK has been started.");

    RELAY_CHANGE_SOURCE change;
    RELAY_STATUS_FLAG = 0;
    uint32_t last_event_time = 0;

    while(1)
    {
        if(xQueueReceive(relay_event_queue, &change, portMAX_DELAY))
        {
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS; //当前的上电以来经历的毫秒数

            if( ( now - last_event_time) > 300 ) {
                //当按钮切换时，两次继电器开关切换之间要间隔300ms，无论是按钮切换还是网络切换
                //对于按钮切换，可以防止机械按钮电平不稳
                //对于网络切换，可以防止网络端切换的时候正好碰上MQTT上报，导致开关瞬间开合
                last_event_time = now; //记录该次触发时间

                RELAY_STATUS_FLAG = !RELAY_STATUS_FLAG; //状态翻转
                RELAY_SET_LEVEL(RELAY_STATUS_FLAG); //设置继电器新状态
                
                if(MQTT_WIFI_CONNECTED_FLAG == 1) 
                    MQTT_RELAY_STATUS_UPDATE(RELAY_STATUS_FLAG);

                if(change == FROM_BUTTON) ESP_LOGI(TAG, "Relay switched from button!");
                else ESP_LOGI(TAG, "Relay switched change from Internet!");
                if(RELAY_STATUS_FLAG == RELAY_OFF) ESP_LOGI(TAG, "Current Relay level: ON -> OFF");
                else ESP_LOGI(TAG, "Current Relay level: OFF -> ON");
            }
        }
    }
}

