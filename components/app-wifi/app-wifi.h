#ifndef __WIFI_H__
#define __WIFI_H__

#include "esp_wifi.h" // IWYU pragma: keep
#include "esp_event.h" // IWYU pragma: keep

#define APPWIFI_ONLINE BIT1
#define APPWIFI_OFFLINE BIT0

// WiFi初始化函数
esp_err_t wifi_init_softap(void);

esp_err_t wifi_reset_connection_retry(void);

//返回当前Wifi是否连接的事件
EventGroupHandle_t appwifi_get_online_event();

// WiFi扫描函数
esp_err_t wifi_scan_networks(wifi_ap_record_t **ap_records, uint16_t *ap_count);

#endif // WIFI_MANAGER_H
