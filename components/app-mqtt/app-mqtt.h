/*
    File: mqtt.h
    Memo: MQTT协议和会话相关
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/
#pragma once

#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MQTT_URI "mqtt://mqtt.jovisdreams.site"
#define MQTT_CLIENT_ID "Smart_Socket_WIFI"
#define MQTT_USERNAME "zhoudreams"
#define MQTT_PASSWD "sbzjx250"

void mqtt_wifi_init();

void mqtt_lte4g_init();

void MQTT_UPDATE_DAEMON();

void MQTT_RELAY_STATUS_UPDATE_WIFI(int level);

void MQTT_RELAY_STATUS_UPDATE_4G(int level);

bool get_mqtt_wifi_connected_flag();

#ifdef __cplusplus
}
#endif