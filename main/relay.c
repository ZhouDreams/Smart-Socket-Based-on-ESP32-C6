/*
    File: relay.h
    Memo: 操作继电器用的函数
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "relay.h"
#include "config.h"

#define TAG "relay.c"

void RELAY_GPIO2_INST()
{
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, RELAY_OFF);
    ESP_LOGI(TAG, "GPIO2 has been installed.");
}

void RELAY_SWITCH_ON()
{
    gpio_set_level(GPIO_NUM_2, RELAY_ON);
}

void RELAY_SWITCH_OFF()
{
    gpio_set_level(GPIO_NUM_2, RELAY_OFF);
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
            uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

            if( (change == FROM_BUTTON && (now - last_event_time) > 200) || change == FROM_INTERNET ) {

                last_event_time = now; //记录该次触发时间

                CURRENT_STATUS = !CURRENT_STATUS; //状态翻转
                gpio_set_level(GPIO_NUM_2, CURRENT_STATUS); //GPIO2输出翻转

                //RELAY_STATUS_UPDATE(CURRENT_STATUS); //更新云端继电器状态

                if(change == FROM_BUTTON) ESP_LOGI(TAG, "Relay level change from button!");
                else ESP_LOGI(TAG, "Relay level change from Internet!");
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
