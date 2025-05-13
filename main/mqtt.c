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
#include "config.h"

#define TAG "mqtt.c"

void MQTT_4G_INST()
{
    ESP_LOGI(TAG, "Initializing MQTT...");

    uart_flush(UART_4G_NUM);

    // SEND_AT_CMD(AT_RESET);
    // vTaskDelay(pdMS_TO_TICKS(1000));

    // SEND_AT_CMD(AT_CPIN);
    // SEND_AT_CMD(AT_CGATT);
    // SEND_AT_CMD(AT_MCONFIG);
    // SEND_AT_CMD(AT_MIPSTART);
    // SEND_AT_CMD(AT_MCONNECT);
    // SEND_AT_CMD(AT_MSUB_RELAY);

    // RELAY_STATUS_UPDATE(RELAY_OFF); //上电后继电器初始为断开

    ESP_LOGI(TAG, "MQTT inst Complete!\n");

}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

esp_mqtt_client_handle_t client_now;

void MQTT_RELAY_STATUS_UPDATE(int level)
{
    char data[3] = "";
    sprintf(data, "%d", level);
    int msg_id = esp_mqtt_client_publish(client_now, "/topic/relay_status", data, 0, 1, 0);
    ESP_LOGI(TAG, "relay_status message published, msg_id=%d, relay = %s", msg_id, data);
}

void MQTT_POWER_UPDATE_TASK()
{    
    while(1)
    {
        char data[10]="\0";
        sprintf(data, "%0.1f", BL0942_POWER);
        int msg_id = esp_mqtt_client_publish(client_now, "/topic/power", data, 0, 1, 0);
        ESP_LOGI(TAG, "Power message published, msg_id=%d, Power = %s", msg_id, data);

        vTaskDelay(pdMS_TO_TICKS(2000));
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

        msg_id = esp_mqtt_client_subscribe(client_now, "/topic/relay_status", 0);
        ESP_LOGI(TAG, "Subscribed topic relay_status, msg_id=%d", msg_id);
        msg_id = esp_mqtt_client_subscribe(client_now, "/topic/power", 0);
        ESP_LOGI(TAG, "Subscribed topic power, msg_id=%d", msg_id);

        MQTT_RELAY_STATUS_UPDATE(RELAY_CURRENT_LEVEL);
        xTaskCreate(MQTT_POWER_UPDATE_TASK, "MQTT_POWER_UPDATE_TASK", 4096, NULL, 2, NULL);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void MQTT_WIFI_INIT()
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_URI,
        .credentials.client_id = MQTT_CLIENT_ID,
        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWD
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

}