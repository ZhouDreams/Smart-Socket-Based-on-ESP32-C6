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

int RELAY_CURRENT_LEVEL = RELAY_OFF;

void RELAY_GPIO2_INST()
{
    gpio_reset_pin(GPIO_RELAY_NUM);
    gpio_set_direction(GPIO_RELAY_NUM, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_RELAY_NUM, RELAY_OFF);
    ESP_LOGI(TAG, "Relay has been installed.");
}

void RELAY_SET_LEVEL(int level)
{
    gpio_set_level(GPIO_RELAY_NUM, level);
}

void RELAY_TASK()
{
    ESP_LOGI(TAG, "RELAY_TASK has been started.");

    RELAY_CHANGE_SOURCE change;
    int CURRENT_STATUS = RELAY_OFF;
    uint32_t last_event_time = 0;

    while(1)
    {
        if(xQueueReceive(relay_event_queue, &change, portMAX_DELAY))
        {
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS; //当前的上电以来经历的毫秒数

            if( ( now - last_event_time) > 200 ) {
                //两次继电器开关切换之间要间隔200ms，无论是按钮切换还是网络切换
                //对于按钮切换，可以防止机械按钮电平不稳
                //对于网络切换，因为MQTT在对订阅过的主题发送信息时，自己也会受到一模一样的信息，防止无限套娃
                last_event_time = now; //记录该次触发时间

                CURRENT_STATUS = !CURRENT_STATUS; //状态翻转
                RELAY_SET_LEVEL(CURRENT_STATUS); //设置继电器新状态
                RELAY_CURRENT_LEVEL = CURRENT_STATUS;
                MQTT_RELAY_STATUS_UPDATE(RELAY_CURRENT_LEVEL);

                if(change == FROM_BUTTON) ESP_LOGI(TAG, "Relay switched from button!");
                else ESP_LOGI(TAG, "Relay switched change from Internet!");
                if(CURRENT_STATUS == RELAY_OFF) ESP_LOGI(TAG, "Current Relay level: ON -> OFF");
                else ESP_LOGI(TAG, "Current Relay level: OFF -> ON");
            }
        }
    }
}

void relay_test()
{
    for(int i=1;i<=300;i++)
    {
        gpio_set_level(GPIO_RELAY_NUM, RELAY_ON);
        ESP_LOGI(TAG, "Relay has been switched to ON.");
        vTaskDelay(pdMS_TO_TICKS(2000));
        gpio_set_level(GPIO_RELAY_NUM, RELAY_OFF);
        ESP_LOGI(TAG, "Relay has been switched to OFF.");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
