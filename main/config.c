/*
    File: config.c
    Memo: 所有跟配置外设有关的函数和变量，所有中断和队列的配置，所有自定义的配置结构体
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "config.h"
#include "wifi.h"

#define TAG "config.c"

uart_config_t uart_config_4G = {
    .baud_rate = UART_4G_BAUD_RATE,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
};

uart_config_t uart_config_BL0942 = {
    .baud_rate = UART_BL0942_BAUD_RATE,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = LP_UART_SCLK_LP_FAST,
};

void UART_4G_INST()
{
    ESP_ERROR_CHECK(uart_driver_install(UART_4G_NUM, BUF_SIZE, BUF_SIZE, 10, &uart0_event_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_4G_NUM, &uart_config_4G));
    ESP_ERROR_CHECK(uart_set_pin(UART_4G_NUM, UART_4G_TX, UART_4G_RX, -1, -1));
    ESP_LOGI(TAG, "UART_4G has been installed.");
}

void UART_BL0942_INST()
{
    ESP_ERROR_CHECK(uart_driver_install(UART_BL0942_NUM, BUF_SIZE, BUF_SIZE, 10, &uart1_event_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_BL0942_NUM, &uart_config_BL0942));
    ESP_ERROR_CHECK(uart_set_pin(UART_BL0942_NUM, UART_BL0942_TX, UART_BL0942_RX, -1, -1));
    ESP_LOGI(TAG, "UART_BL0942 has been installed.\n");
}

void GPIO2_INST()
{
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, RELAY_OFF);
    ESP_LOGI(TAG, "GPIO2 has been installed.");
}

void GPIO3_INST()
{
    gpio_set_direction(GPIO_NUM_3, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_3, GPIO_PULLDOWN_ONLY);
    gpio_set_intr_type(GPIO_NUM_3, GPIO_INTR_POSEDGE); //设置中断为上升沿触发

    ESP_LOGI(TAG, "GPIO3 has been installed.");
}

void WIFI_INIT()
{
    ESP_ERROR_CHECK(esp_netif_init());
    EventGroupHandle_t wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
}

void IRAM_ATTR BUTTON_ISR_HANDLER(void* arg)
{
    RELAY_CHANGE_SOURCE change = FROM_BUTTON;
    xQueueSendFromISR(relay_event_queue, &change, NULL);
}