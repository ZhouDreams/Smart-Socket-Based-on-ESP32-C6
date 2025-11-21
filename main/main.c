/*
    File: main.c
    Memo: 主程序入口
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include <string.h> // IWYU pragma: keep
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h" // IWYU pragma: keep
#include "driver/uart.h" // IWYU pragma: keep
#include "lte4g.h"
#include "bl0942.h"
#include "button-and-relay.h"
#include "http-server.h"
#include "app-wifi.h"
#include "app-mqtt.h"
#include "config.h" // IWYU pragma: keep

#define PRIORITY_NORMAL 1
#define PRIORITY_REALTIME 10

static const char* TAG = "main";

void setup()
{
    ESP_LOGI(TAG, "Enter setup().");

//----------初始化继电器和按钮----------
    ESP_ERROR_CHECK(relay_gpio_inst());
    ESP_ERROR_CHECK(button_gpio_inst());
    relay_task_start(PRIORITY_REALTIME);     //启动继电器任务

//----------初始化BL0942计量模块----------
    ESP_ERROR_CHECK(bl0942_uart_inst());
    bl0942_task_start(PRIORITY_NORMAL);
    
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
    lte4g_module_init_task_start(1);
    vTaskDelay(pdMS_TO_TICKS(200));

//----------初始化MQTT----------
    appmqtt_wifi_init_task_start(PRIORITY_NORMAL);
    appmqtt_wifi_update_task_start(PRIORITY_NORMAL);
    appmqtt_lte4g_init_task_start(PRIORITY_NORMAL);
    appmqtt_lte4g_update_task_start(PRIORITY_NORMAL);

    ESP_LOGI(TAG, "Setup() returns.");

}

void app_main()
{
    setup();


    ESP_LOGI(TAG, "app_main() returns.");

}

