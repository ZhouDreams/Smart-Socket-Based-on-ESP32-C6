/*
    File: mqtt.c
    Memo: MQTT协议和会话相关
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "bl0942.h"
#include "lte4g.h"
#include "app-wifi.h"
#include "button-and-relay.h"
#include "app-mqtt.h"

#define TAG "app-mqtt"

typedef struct 
{
    bool online;
    bool network;
    bool power;
    bool relay_status;
} appmqtt_update_task_t;

typedef struct 
{
    bool MCONFIG;
    bool MIPSTART;
    bool MCONNECT;
    bool MSUB;
} appmqtt_lte4g_init_task_t;

static esp_mqtt_client_config_t mqtt_wifi_cfg = {
        .broker.address.uri = MQTT_URI,
        .credentials.client_id = MQTT_CLIENT_ID,
        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWD,
        .network.disable_auto_reconnect = false,
        .network.reconnect_timeout_ms = 5000,
        .session.keepalive = 5,
        .session.last_will.topic = "/topic/online",
        .session.last_will.msg = "0",
        .session.last_will.msg_len = 1,
        .session.last_will.qos = 0,
        .session.last_will.retain = 0,
        .broker.verification.skip_cert_common_name_check = true

    };

static esp_mqtt_client_handle_t client_now; //当前MQTT客户端句柄
static bool s_mqtt_wifi_connected_flag;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void appmqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    client_now = client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        s_mqtt_wifi_connected_flag = 1;
        ESP_LOGW(TAG, "MQTT_EVENT_CONNECTED");
        
        msg_id = esp_mqtt_client_subscribe(client_now, "/topic/relay_status_ctrl", 0);
        ESP_LOGI(TAG, "Subscribed topic relay_status, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client_now, "/topic/power_thresh_ctrl", 0);
        ESP_LOGI(TAG, "Subscribed topic power_thresh_ctrl, msg_id=%d", msg_id);

        MQTT_RELAY_STATUS_UPDATE_WIFI(relay_get_level());
        
        break;

    case MQTT_EVENT_DISCONNECTED:
        s_mqtt_wifi_connected_flag = 0;
        lte4g_send_at_no_print("AT+MQTTMSGGET\r\n");
        ESP_LOGW(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGW(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGW(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        //ESP_LOGW(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGW(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        if( strstr(event->topic, "/topic/relay_status_ctrl") != NULL){

            // relay_waiter.new_status = !relay_get_level();
            // relay_waiter.source = FROM_INTERNET;
            // xSemaphoreGive(relay_waiter.change_sig);
        }
        else if( strstr(event->topic, "/topic/power_thresh_ctrl") != NULL){
            
            // POWER_THRESH = atoi(event->data);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGW(TAG, "MQTT_EVENT_ERROR");
        s_mqtt_wifi_connected_flag = 0;
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGW(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void appmqtt_wifi_init(int priority)
{
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_wifi_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, appmqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

static void appmqtt_lte4g_init_task()
{
    char response[BUF_SIZE] = "\0";
    memset(response,0,sizeof(response));
    appmqtt_lte4g_init_task_t appmqtt_lte4g_init_task = {0,0,0,0};

    xEventGroupWaitBits(lte4g_get_online_event(), LTE4G_ONLINE, pdFALSE, pdTRUE, portMAX_DELAY);

    while (1) {
        //设置MQTT相关参数
        if(appmqtt_lte4g_init_task.MCONFIG == 0){
            memset(response,0,sizeof(response));
            strcpy(response, lte4g_send_at_cmd(AT_MCONFIG));
            if( strstr(response, "OK") == NULL) continue;
            appmqtt_lte4g_init_task.MCONFIG = 1;
            ESP_LOGI(TAG, "MQTT Config Configured.");
        }
    
        //建立TCP连接
        if(appmqtt_lte4g_init_task.MIPSTART == 0){
            memset(response,0,sizeof(response));
            strcpy(response, lte4g_send_at_cmd(AT_MIPSTART));
            if( strstr(response, "OK") == NULL) continue;
            appmqtt_lte4g_init_task.MIPSTART = 1;
            ESP_LOGI(TAG, "TCP Connection Started.");
        }

        //客户端向服务器请求会话连接
        if(appmqtt_lte4g_init_task.MCONNECT == 0){
            //lte4g_send_at_cmd(AT_MDISCONNECT);
            memset(response,0,sizeof(response));
            strcpy(response, lte4g_send_at_cmd(AT_MCONNECT));
            if( strstr(response, "OK") == NULL) continue;
            appmqtt_lte4g_init_task.MCONNECT = 1;
            ESP_LOGI(TAG, "MQTT Connection Started.");
        }

        //订阅主题
        if(appmqtt_lte4g_init_task.MSUB == 0) {
            char cmd[50];    
            sprintf(cmd, "AT+MSUB=\"25108143g/relay_status_ctrl\",0\r\n");
            memset(response,0,sizeof(response));
            strcpy(response, lte4g_send_at_cmd(cmd));
            if(strstr(response,"OK") != NULL) ESP_LOGI(TAG, "25108143g/relay_status_ctrl subscribed.");
            else
            {
                ESP_LOGI(TAG, "25108143g/relay_status_ctrl subscribe failed!");
                continue;
            }
            appmqtt_lte4g_init_task.MSUB = 1;
            ESP_LOGI(TAG, "MQTT Subscribe Success.");
        }

    //     sprintf(cmd, "AT+MSUB=\"/topic/power_thresh_ctrl\",0\r\n");
    //     memset(response,0,sizeof(response));
    //     strcpy(response, lte4g_send_at_no_print(cmd, AT_RESPONSE_DELAY));
    //     if(strstr(response,"OK") != NULL) ESP_LOGI(TAG, "/topic/power_thresh_ctrl subscribed.");
    //     else
    //     {
    //         ESP_LOGI(TAG, "topic/power_thresh_ctrl subscribe failed!");
    //         continue;
    //     }
        break;
    }
    ESP_LOGI(TAG, "lte4g ready.");
    vTaskDelete(NULL);
}

void appmqtt_lte4g_init(int priority)
{
    xTaskCreate(appmqtt_lte4g_init_task, "appmqtt_lte4g_init_task", 4096, NULL, priority, NULL);
}

static void appmqtt_lte4g_update_task()
{
    while (1)
    {
        xEventGroupWaitBits(appwifi_get_online_event(), APPWIFI_OFFLINE, pdFALSE, pdTRUE, portMAX_DELAY);
        xEventGroupWaitBits(lte4g_get_online_event(), LTE4G_ONLINE, pdFALSE, pdTRUE, portMAX_DELAY);
        if ((xEventGroupGetBits(appwifi_get_online_event()) & APPWIFI_OFFLINE) == 0) continue;
        
        ESP_LOGW(TAG, "WIFI disconnected, MQTT Updating through 4G.");
        char cmd[50];
        char response[BUF_SIZE] = "\0";

        appmqtt_update_task_t mqtt_update_task = {0,0,0,0};

        //发送online心跳
        sprintf(cmd, "AT+MPUB=\"25108143g/online\",0,0,\"%s\"\r\n","1");
        memset(response,0,sizeof(response));
        strcpy(response, lte4g_send_at_no_print(cmd));
        if(strstr(response,"OK") != NULL) mqtt_update_task.online = 1;
        else ESP_LOGI(TAG, "Online status update failed!");

        //上报功耗信息
        char power[10]="\0";
        sprintf(power, "%0.1fW", bl0942_get_power());
        sprintf(cmd, "AT+MPUB=\"25108143g/power\",0,0,\"%s\"\r\n",power);
        memset(response,0,sizeof(response));
        strcpy(response, lte4g_send_at_no_print(cmd));
        if (strstr(response,"OK") != NULL) mqtt_update_task.power = 1;
        else ESP_LOGI(TAG, "Power update failed!");

           //上报继电器状态
        char relay_status[3]="\0";
        sprintf(relay_status, "%d", relay_get_level());
        sprintf(cmd, "AT+MPUB=\"25108143g/relay_status\",0,0,\"%s\"\r\n",relay_status);
        memset(response,0,sizeof(response));
        strcpy(response, lte4g_send_at_no_print(cmd));
        if(strstr(response,"OK") != NULL) mqtt_update_task.relay_status = 1;
        else ESP_LOGI(TAG, "Relay status update failed!");

        //上报当前网络
        char network[3]="4G";
        sprintf(cmd, "AT+MPUB=\"25108143g/network\",0,0,\"%s\"\r\n",network);
        memset(response,0,sizeof(response));
        strcpy(response, lte4g_send_at_no_print(cmd));
        if(strstr(response,"OK") != NULL) mqtt_update_task.network = 1;
        else ESP_LOGI(TAG, "Network update failed!");

        ESP_LOGI(TAG, "online, relay_status = %s, power = %s, network = %s updated.", relay_status, power, network);
        vTaskDelay(pdMS_TO_TICKS(2000));
    } 
}

void appmqtt_lte4g_update_task_start(int priority)
{
    xTaskCreate( appmqtt_lte4g_update_task, "appmqtt_lte4g_update_task", 4096, NULL, priority, NULL);
}

void appmqtt_lte4g_relay_update(RelayTargetLevel_t level)
{
    char cmd[50];
    char response[BUF_SIZE] = "\0";
    char relay_status[3]="\0";
    sprintf(relay_status, "%d", relay_get_level());
    sprintf(cmd, "AT+MPUB=\"25108143g/relay_status\",0,0,\"%s\"\r\n",relay_status);
    memset(response,0,sizeof(response));
    strcpy(response, lte4g_send_at_no_print(cmd));
    if(strstr(response,"OK") != NULL) ESP_LOGI(TAG, "Relay status change updated through 4g.");
    else ESP_LOGI(TAG, "Relay status update failed!");
}

void appmqtt_lte4g_msub_handler(const char* line)
{
    if (strstr(line, "relay_status_ctrl"))
    {
        RelayCMD_t relay_cmd = {
        .relay_op_source = SRC_LTE4G,
        .relay_op_type = TOGGLE,
        .op_tick = xTaskGetTickCount()
        };
        relay_send_cmd(relay_cmd);
    }
}

void MQTT_RELAY_STATUS_UPDATE_WIFI(int level)
{
    char data[3] = "";
    sprintf(data, "%d", level);
    int msg_id = esp_mqtt_client_publish(client_now, "/topic/relay_status", data, 0, 1, 0);
    ESP_LOGI(TAG, "relay_status message published, msg_id=%d, relay = %s", msg_id, data);
}

// void MQTT_UPDATE_DAEMON()
// //MQTT周期上报任务
// {    
//     ESP_LOGI(TAG, "MQTT_UPDATE_DAEMON() Started.");
//     while(1)
//     {
//         if(s_mqtt_wifi_connected_flag == 1)
//         //如果通过WIFI连接的MQTT初始化完成，则通过WIFI上报MQTT服务器
//         {
//             int msg_id;

//             msg_id = esp_mqtt_client_publish(client_now, "/topic/online", "1", 0, 1, 0);
            
//             char power[10]="\0";
//             sprintf(power, "%0.1fW", bl0942_get_power());
//             msg_id = esp_mqtt_client_publish(client_now, "/topic/power", power, 0, 1, 0);
//             //ESP_LOGI(TAG, "Power message published, msg_id=%d, Power = %s", msg_id, data);

//             char relay_status[3]="\0";
//             sprintf(relay_status, "%d", relay_get_level());
//             msg_id = esp_mqtt_client_publish(client_now, "/topic/relay_status", relay_status, 0, 1, 0);
//             //ESP_LOGI(TAG, "Relay_status message published, msg_id=%d, relay_status = %s", msg_id, data);

//             char network[6]="Wi-Fi";
//             msg_id = esp_mqtt_client_publish(client_now, "/topic/network", network, 0, 1, 0);

//             char power_thresh[10];
//             sprintf(power_thresh, "%d", bl0942_get_power_thresh());
//             msg_id = esp_mqtt_client_publish(client_now, "/topic/power_thresh", power_thresh, 0, 1, 0);

//             ESP_LOGI(TAG, "MQTT Updated through WIFI.");
//             ESP_LOGI(TAG, "Power = %s, Relay_Status = %s, Network = %s, power_thresh = %s.",power,relay_status,network,power_thresh);
//         }

//         else if(lte4g_get_online() == 1)
//         //如果MQTT未通过WIFI连接，但4G模块在线，则通过4G上报MQTT服务器
//         {
            
//         }
//         else
//         //如果WIFI和4G都不通，那就寄了
//         {
//             ESP_LOGE(TAG, "WIFI and 4G both disconnected, MQTT update failed!");
//         }

//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

bool get_mqtt_wifi_connected_flag()
{
    return s_mqtt_wifi_connected_flag;
}
