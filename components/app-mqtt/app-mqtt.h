/*
    File: mqtt.h
    Memo: MQTT协议和会话相关
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/
#pragma once

#include "driver/uart.h" // IWYU pragma: keep
#include "button-and-relay.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MQTT_URI "mqtt://broker.emqx.io"
#define MQTT_CLIENT_ID "25108143g-wifi"
#define MQTT_USERNAME ""
#define MQTT_PASSWD ""

//--------------------------------wifi上报mqtt--------------------------------
//wifi上报mqtt的初始化
void appmqtt_wifi_init_task_start(int priority);

//获取wifi上报mqtt的在线状态
bool appmqtt_get_wifi_connected_flag();

//启动通过wifi上报mqtt的任务
void appmqtt_wifi_update_task_start(int priority);

//通过wifi更新继电器状态
esp_err_t appmqtt_wifi_relay_update(RelayTargetLevel_t level);

//通过wifi手动更新数据
esp_err_t appmqtt_wifi_update_manually(const char* topic, const char* data);

//--------------------------------4g上报mqtt--------------------------------
//4g上报mqtt的初始化
void appmqtt_lte4g_init_task_start(int priority);

//获取4g上报mqtt的在线状态
bool appmqtt_get_lte4g_connected_flag();

//启动通过4g上报mqtt的任务
void appmqtt_lte4g_update_task_start(int priority);

//通过4g更新继电器状态
esp_err_t appmqtt_lte4g_relay_update(RelayTargetLevel_t level);

QueueHandle_t appmqtt_get_lte4g_msub_queue();

#ifdef __cplusplus
}
#endif