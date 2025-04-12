/*
    File: main.c
    Memo: 主程序入口
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "4G.h"
#include "BL0942.h"
#include "button.h"
#include "relay.h"
#include "config.h"

#define TAG "main.c"


SYNC_DATA_t DATA_TEST={
    .relay_status = 1,
    .battery_or_dc = 0,
    .battery_voltage_percent = 72,
    .power_thresh = 999,
    .power = 25.6,
    .WIFI_or_4G = 0
  
};

QueueHandle_t relay_event_queue = NULL; 
QueueHandle_t uart0_event_queue = NULL; 
QueueHandle_t uart1_event_queue = NULL; 

void setup()
{
    ESP_LOGI(TAG, "Enter setup.");

    GPIO2_INST();
    GPIO3_INST();

    UART_4G_INST();
    xTaskCreate(AIR780EP_INST, "AIR780EP_INST", 4096, NULL, 1, NULL);

    UART_BL0942_INST();
    
    
    // relay_event_queue = xQueueCreate(10,sizeof(RELAY_CHANGE_SOURCE)); //创建继电器事件队列
    // gpio_install_isr_service(0); //安装GPIO ISR服务
    // gpio_isr_handler_add(GPIO_NUM_3, BUTTON_ISR_HANDLER, NULL); //添加按钮的GPIO中断
    // xTaskCreate(RELAY_TASK, "relay_task", 4096, NULL, 10, NULL); //启动任务

    // xTaskCreate(UART1_EVENT_TASK, "uart1_event_task", 4096, NULL, 10, NULL);

    // MQTT_INST();

    ESP_LOGI(TAG, "Setup() returns.");

}

void app_main()
{
    setup();

    xTaskCreate(BL0942_READ_TASK, "BL0942_READ_TASK", 4096, NULL, 2, NULL);

    ESP_LOGI(TAG, "app_main() returns.");

}