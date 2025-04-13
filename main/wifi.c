/*
    File: wifi.c
    Memo: WIFI相关
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include <string.h>
#include "wifi.h"
#include "config.h"

#define TAG "wifi.c"

char WIFI_SSID[] = "LoveGoldenGlow";
char WIFI_PASSWD[] = "sbzjx250";

void WIFI_INIT()
{
    
}

