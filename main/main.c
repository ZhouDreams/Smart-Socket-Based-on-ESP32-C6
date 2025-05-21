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
#include "nvs_flash.h"
#include "4G.h"
#include "BL0942.h"
#include "button.h"
#include "relay.h"
#include "http_server.h"
#include "wifi_manager.h"
#include "mqtt.h"
#include "config.h"

#define TAG "main.c"

QueueHandle_t relay_event_queue = NULL; 

void setup()
{
    ESP_LOGI(TAG, "Enter setup().");

//----------初始化继电器和按钮----------
    RELAY_GPIO2_INST();
    BUTTON_GPIO3_INST();
    relay_event_queue = xQueueCreate(10,sizeof(RELAY_CHANGE_SOURCE)); //创建继电器事件队列
    gpio_install_isr_service(0); //安装GPIO ISR服务
    gpio_isr_handler_add(GPIO_BUTTON_NUM, BUTTON_ISR_HANDLER, NULL); //添加按钮的GPIO中断
    xTaskCreate(RELAY_TASK, "RELAY_TASK", 4096, NULL, 10, NULL); //启动继电器任务

//----------初始化BL0942计量模块----------
    UART_BL0942_INST();
    xTaskCreate(BL0942_READ_TASK, "BL0942_READ_TASK", 4096, NULL, 2, NULL); //启动从BL0942周期读取电量数据的任务
    
//----------初始化WIFI----------
    WIFI_GPIO18_INIT(); //初始化WIFI连接状态指示灯

    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 初始化SPIFFS
    ESP_ERROR_CHECK(init_spiffs());

    ESP_LOGI(TAG, "Starting WiFi in AP mode");
    ESP_ERROR_CHECK(wifi_init_softap());

    start_webserver();

//----------初始化4G模块----------
    UART_4G_INST();
    
    xTaskCreate(AIR780EP_INST, "AIR780EP_INST", 4096, NULL, 1, NULL);

//----------初始化MQTT----------
    while (1)
    {
        //等待WIFI和4G其中任一连上MQTT服务器
        if(WIFI_CONNECTED_FLAG == 1 || Air780EP_ONLINE_FLAG == 1)
        {
            MQTT_WIFI_INIT();
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
    while (1)
    {
        if(MQTT_WIFI_CONNECTED_FLAG == 1 || Air780EP_ONLINE_FLAG == 1)
        {
            xTaskCreate(MQTT_UPDATE_DAEMON, "MQTT_UPDATE_DAEMON", 4096, NULL, 2, NULL);
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
    



    ESP_LOGI(TAG, "Setup() returns.");

}

void app_main()
{
    setup();


    ESP_LOGI(TAG, "app_main() returns.");

}

