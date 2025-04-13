/*
    File: button.c
    Memo: 控制按钮
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "button.h"
#include "config.h"

#define TAG "button.c"

void BUTTON_GPIO3_INST()
{
    gpio_set_direction(GPIO_NUM_3, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_3, GPIO_PULLDOWN_ONLY);
    gpio_set_intr_type(GPIO_NUM_3, GPIO_INTR_POSEDGE); //设置中断为上升沿触发

    ESP_LOGI(TAG, "GPIO3 has been installed.");
}

void IRAM_ATTR BUTTON_ISR_HANDLER(void* arg)
{
    RELAY_CHANGE_SOURCE change = FROM_BUTTON;
    xQueueSendFromISR(relay_event_queue, &change, NULL);
}