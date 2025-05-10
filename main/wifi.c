/*
    File: wifi.c
    Memo: WIFI相关
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "wifi.h"
#include "config.h"

static EventGroupHandle_t wifi_event_group; //WIFI事件组

#define TAG "wifi.c"

static int s_retry_num = 0; //WIFI连接失败后已重试次数

//WIFI事件处理函数
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    //STA模式的WIFI启动时连接WIFI
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } 
    //WIFI断开连接时，如果没有达到最大重试次数，则重新连接；如果达到最大重试次数则将WIFI的事件组设为失败
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying to connect to the AP...");
        }
        else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI(TAG,"Connection to the AP failed.");
        }
    }
    //获取IP
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static wifi_config_t wifi_config_ap = {
    .ap = {
        .ssid = "ESP32-C6",
        .ssid_len = strlen("ESP32-C6"),
        .channel = 1,
        .password = "",
        .max_connection = 10,
        .authmode = WIFI_AUTH_OPEN,
    }
};

static wifi_config_t wifi_config_sta = {
    .sta = {
        .ssid = WIFI_SSID, //要连接的WIFI SSID
        .password = WIFI_PASSWD, //要连接的WIFI密码
        .threshold.authmode = WIFI_AUTH_WPA2_PSK, //使用WPA2安全协议
        .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
    }
};

void WIFI_INIT()
{
//--------------------初始化阶段--------------------

    ESP_ERROR_CHECK(esp_netif_init()); //初始化底层TCP/IP栈
    ESP_ERROR_CHECK(esp_event_loop_create_default()); //创建系统事件任务
    //创建有 TCP/IP 堆栈的默认网络接口实例绑定Staton和AP
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
    //初始化 Wi-Fi 驱动程序
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

//--------------------配置事件组--------------------

    wifi_event_group = xEventGroupCreate(); //创建事件组
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    //实例化事件处理函数
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    ESP_LOGI(TAG, "WIFI_INIT finished.");

    
}

void WIFI_AP_START()
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP) ); //设置WIFI为Station模式
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap) ); //配置WIFI
    ESP_ERROR_CHECK(esp_wifi_start() ); //启动WIFI
    ESP_LOGI(TAG, "WIFI AP started.");
}

void WIFI_AP_STOP()
{
    esp_wifi_stop();
    ESP_LOGI(TAG, "WIFI AP stopped.");
}

void WIFI_STA_START()
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) ); //设置WIFI为Station模式
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta) ); //配置WIFI
    ESP_ERROR_CHECK(esp_wifi_start() ); //启动WIFI
    ESP_LOGI(TAG, "WIFI STA started.");

        //等待wifi事件组传来连接成功或连接失败的消息
        EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASSWD);
    }
    else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASSWD);
    }
    else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

void WIFI_STA_STOP()
{
    esp_wifi_disconnect();
    esp_wifi_stop();
}

void NVS_INIT()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}
