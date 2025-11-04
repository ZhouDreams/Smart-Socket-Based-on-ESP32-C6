/*
    File: main.c
    Memo: 主程序入口
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "lte4g.h"
#include "bl0942.h"
#include "button-and-relay.h"
#include "http-server.h"
#include "app-wifi.h"
#include "app-mqtt.h"
#include "config.h"

#define TAG "main"

void setup()
{
    ESP_LOGI(TAG, "Enter setup().");

//----------初始化继电器和按钮----------
    relay_gpio_inst();
    button_gpio_inst();
    relay_task_start();     //启动继电器任务

//----------初始化BL0942计量模块----------
    bl0942_uart_inst();
    bl0942_task_start();
    
//----------初始化WIFI----------

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
    lte4g_uart_inst();
    lte4g_rx_task_start();
    lte4g_software_inst_start();

//----------初始化MQTT----------
    while (1)
    {
        //等待WIFI和4G其中任一连上MQTT服务器
        if(appwifi_get_connected() == 1 || lte4g_get_online() == 1)
        {
            mqtt_wifi_init();
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
    while (1)
    {
        if(get_mqtt_wifi_connected_flag() == 1 || lte4g_get_online() == 1)
        {
            xTaskCreate(MQTT_UPDATE_DAEMON, "MQTT_UPDATE_DAEMON", 4096, NULL, 1, NULL);
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

