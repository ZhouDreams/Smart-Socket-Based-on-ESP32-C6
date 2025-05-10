/*
    File: wifi.h
    Memo: WIFI相关
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#ifndef __WIFI_H__
#define __WIFI_H__

#define WIFI_SSID      "米哈游株式会社"
#define WIFI_PASSWD      "genshin666"
#define WIFI_MAXIMUM_RETRY  10

//事件位，WIFI已连接是BIT0，WIFI连接失败是BIT1
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#define CONFIG_ESP_WIFI_PW_ID ""

void WIFI_INIT();
void WIFI_AP_START();
void WIFI_AP_STOP();
void WIFI_STA_START();
void WIFI_STA_STOP();
int NVS_WIFI_CHECK();
void NVS_INIT();

#endif