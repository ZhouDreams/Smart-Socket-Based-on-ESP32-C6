/*
    File: mqtt.c
    Memo: MQTT协议和会话相关
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "BL0942.h"
#include "relay.h"
#include "4G.h"
#include "config.h"

#define TAG "mqtt.c"

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

esp_mqtt_client_handle_t client_now; //当前MQTT客户端句柄

void MQTT_RELAY_STATUS_UPDATE(int level)
{
    char data[3] = "";
    sprintf(data, "%d", level);
    int msg_id = esp_mqtt_client_publish(client_now, "/topic/relay_status", data, 0, 1, 0);
    ESP_LOGI(TAG, "relay_status message published, msg_id=%d, relay = %s", msg_id, data);
}

void MQTT_UPDATE_DAEMON()
//MQTT周期上报任务
{    
    ESP_LOGI(TAG, "MQTT_UPDATE_DAEMON() Started.");
    while(1)
    {
        if(MQTT_WIFI_CONNECTED_FLAG == 1)
        //如果通过WIFI连接的MQTT初始化完成，则通过WIFI上报MQTT服务器
        {
            int msg_id;

            msg_id = esp_mqtt_client_publish(client_now, "/topic/online", "1", 0, 1, 0);
            
            char power[10]="\0";
            sprintf(power, "%0.1fW", BL0942_POWER);
            msg_id = esp_mqtt_client_publish(client_now, "/topic/power", power, 0, 1, 0);
            //ESP_LOGI(TAG, "Power message published, msg_id=%d, Power = %s", msg_id, data);

            char relay_status[3]="\0";
            sprintf(relay_status, "%d", RELAY_STATUS_FLAG);
            msg_id = esp_mqtt_client_publish(client_now, "/topic/relay_status", relay_status, 0, 1, 0);
            //ESP_LOGI(TAG, "Relay_status message published, msg_id=%d, relay_status = %s", msg_id, data);

            char network[6]="Wi-Fi";
            msg_id = esp_mqtt_client_publish(client_now, "/topic/network", network, 0, 1, 0);

            char power_thresh[10];
            sprintf(power_thresh, "%d", POWER_THRESH);
            msg_id = esp_mqtt_client_publish(client_now, "/topic/power_thresh", power_thresh, 0, 1, 0);

            ESP_LOGI(TAG, "MQTT Updated through WIFI.");
            ESP_LOGI(TAG, "Power = %s, Relay_Status = %s, Network = %s, power_thresh = %s.",power,relay_status,network,power_thresh);
        }

        else if(Air780EP_ONLINE_FLAG == 1)
        //如果MQTT未通过WIFI连接，但4G模块在线，则通过4G上报MQTT服务器
        {
            ESP_LOGW(TAG, "WIFI disconnected, MQTT Updating through 4G.");
            char cmd[50];
            char response[BUF_SIZE] = "\0";

            //检查缓存里有没有订阅信息
            sprintf(cmd, "AT+MQTTMSGGET\r\n");
            memset(response,0,sizeof(response));
            strcpy(response, SEND_AT_CMD_NO_PRINT(cmd, AT_RESPONSE_DELAY));
            if(strstr(response, "+MSUB: \"/topic/relay_status_ctrl\",1 byte,1") != NULL)
            {
                RELAY_CHANGE_SOURCE change = FROM_INTERNET;
                xQueueSendFromISR(relay_event_queue, &change, NULL);
            }

            //发送online心跳
            sprintf(cmd, "AT+MPUB=\"/topic/online\",0,0,\"%s\"\r\n","1");
            memset(response,0,sizeof(response));
            strcpy(response, SEND_AT_CMD_NO_PRINT(cmd, AT_RESPONSE_DELAY));
            if(strstr(response,"OK") != NULL) ESP_LOGI(TAG, "Online status updated.");
            else ESP_LOGI(TAG, "Power update failed!");

            //上报功耗信息
            char power[10]="\0";
            sprintf(power, "%0.1fW", BL0942_POWER);
            sprintf(cmd, "AT+MPUB=\"/topic/power\",0,0,\"%s\"\r\n",power);
            memset(response,0,sizeof(response));
            strcpy(response, SEND_AT_CMD_NO_PRINT(cmd, AT_RESPONSE_DELAY));
            if(strstr(response,"OK") != NULL) ESP_LOGI(TAG, "Power = %s updated.", power);
            else ESP_LOGI(TAG, "Power update failed!");

            //上报继电器状态
            char relay_status[3]="\0";
            sprintf(relay_status, "%d", RELAY_STATUS_FLAG);
            sprintf(cmd, "AT+MPUB=\"/topic/relay_status\",0,0,\"%s\"\r\n",relay_status);
            memset(response,0,sizeof(response));
            strcpy(response, SEND_AT_CMD_NO_PRINT(cmd, AT_RESPONSE_DELAY));
            if(strstr(response,"OK") != NULL) ESP_LOGI(TAG, "Relay status = %s updated.", relay_status);
            else ESP_LOGI(TAG, "Relay status update failed!");

            //上报当前网络
            char network[3]="4G";
            sprintf(cmd, "AT+MPUB=\"/topic/network\",0,0,\"%s\"\r\n",network);
            memset(response,0,sizeof(response));
            strcpy(response, SEND_AT_CMD_NO_PRINT(cmd, AT_RESPONSE_DELAY));
            if(strstr(response,"OK") != NULL) ESP_LOGI(TAG, "Network = %s updated.", network);
            else ESP_LOGI(TAG, "Network update failed!");

        }
        else
        //如果WIFI和4G都不通，那就寄了
        {
            ESP_LOGE(TAG, "WIFI and 4G both disconnected, MQTT update failed!");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    client_now = client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        MQTT_WIFI_CONNECTED_FLAG = 1;
        ESP_LOGW(TAG, "MQTT_EVENT_CONNECTED");
        
        msg_id = esp_mqtt_client_subscribe(client_now, "/topic/relay_status_ctrl", 0);
        ESP_LOGI(TAG, "Subscribed topic relay_status, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client_now, "/topic/power_thresh_ctrl", 0);
        ESP_LOGI(TAG, "Subscribed topic power_thresh_ctrl, msg_id=%d", msg_id);

        MQTT_RELAY_STATUS_UPDATE(RELAY_STATUS_FLAG);
        
        break;

    case MQTT_EVENT_DISCONNECTED:
        MQTT_WIFI_CONNECTED_FLAG = 0;
        SEND_AT_CMD_NO_PRINT("AT+MQTTMSGGET\r\n", AT_RESPONSE_DELAY);
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

            RELAY_CHANGE_SOURCE change = FROM_INTERNET;
            xQueueSendFromISR(relay_event_queue, &change, NULL);
        }
        else if( strstr(event->topic, "/topic/power_thresh_ctrl") != NULL){
            
            POWER_THRESH = atoi(event->data);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGW(TAG, "MQTT_EVENT_ERROR");
        MQTT_WIFI_CONNECTED_FLAG = 0;
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

void MQTT_WIFI_INIT()
{
    esp_mqtt_client_config_t mqtt_cfg = {
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

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

}