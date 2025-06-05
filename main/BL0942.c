/*
    File: BL0942.c
    Memo: 计量模块
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include "freertos/FreeRTOS.h"
#include <math.h>
#include "driver/uart.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "BL0942.h"

#include "config.h"

#define TAG "BL0942.c"

float BL0942_POWER = 0; //全局变量，BL0942检测到的有功功率
int POWER_THRESH = 2500; //全局变量，用户设置的功率限制，默认为2500W
int POWER_ACCUMULATION = 0; //全局变量，已用电量

//BL0942 IC初始化
void UART_BL0942_INST()
{
    ESP_ERROR_CHECK(uart_driver_install(UART_BL0942_NUM, BUF_SIZE, BUF_SIZE, 10, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_BL0942_NUM, &uart_config_BL0942));
    ESP_ERROR_CHECK(uart_set_pin(UART_BL0942_NUM, UART_BL0942_TX, UART_BL0942_RX, -1, -1));
    ESP_LOGI(TAG, "UART_BL0942 has been installed.\n");
}

//BL0942电量计量任务
void BL0942_READ_TASK()
{
    char cmd = '\0';
    uint32_t v_rms_raw = 0;
    uint32_t a_rms_raw = 0;
    uint32_t p_rms_raw = 0;
    float voltage_AC_IN = 0;
    float current_OUT = 0;
    float power_LOAD = 0;

    while(1)
    {
    //清除FIFO中可能存在的乱七八糟的缓存
        uart_flush(UART_BL0942_NUM);

    //发送读取所有数据的指令
        cmd = BL0942_READ_CMD;
        uart_write_bytes(UART_BL0942_NUM, &cmd, strlen(&cmd));
        cmd = BL0942_READ_ALL;
        uart_write_bytes(UART_BL0942_NUM, &cmd, strlen(&cmd));

        vTaskDelay(pdMS_TO_TICKS(500));

    //读取模块回复
        char response[50] = "\0";
        int len = uart_read_bytes(UART_BL0942_NUM, response, BUF_SIZE - 1, 10);

        // ESP_LOGI(TAG, "Response length %d: ",len);
        // for(int i=0; i<len; i++)
        // {
        //     printf("%02X ",response[i]); //'X'是指输出十六进制，'2'是指变成两位，'0'是指用0填充
        // }
        // printf("\n");

        if(len != 23)
        {
            ESP_LOGI(TAG, "Response Length illegal! retrying...");
            vTaskDelay(pdMS_TO_TICKS(1500));
            continue;
        }

        v_rms_raw = (response[6] << 16) | (response[5] << 8) | response[4]; 
        voltage_AC_IN = (v_rms_raw * 1.218*( 390*5 + 0.51 )) / (73989*0.51*1000);//3.824=（R1+R2）/R1*1000
        a_rms_raw = (response[3] << 16) | (response[2] << 8) | response[1]; 
        current_OUT =  (a_rms_raw * 1.218 ) / (305978/(0.001*1000));
        p_rms_raw = (response[12] << 16) | (response[11] << 8) | response[10];
        power_LOAD = (p_rms_raw * 1.218 * 1.218 * (390*5 + 0.51) ) / (3537 * 1 * 0.51 * 1000);
        if(power_LOAD > 10000) continue; //在继电器切换的时候有时会出现20000多瓦的异常数据，原因不明，先治标再说

        BL0942_POWER = roundf(power_LOAD*10)/10;
        if(BL0942_POWER > POWER_THRESH)
        {
            RELAY_CHANGE_SOURCE change = FROM_BUTTON;
            xQueueSendFromISR(relay_event_queue, &change, NULL);
            ESP_LOGE(TAG, "Power exceeds the limit! Shut the relay.");
        }
        //ESP_LOGI(TAG, "Voltage: %0.1fV, Current: %0.1fA, Power: %0.1fW",voltage_AC_IN, current_OUT, BL0942_POWER);
        

        vTaskDelay(pdMS_TO_TICKS(500));

    } 

}

