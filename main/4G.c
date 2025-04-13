/*
    File: 4G.c
    Memo: 控制和4G模块的通信
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "esp_log.h"
#include "string.h"
#include "4G.h"
#include "config.h"

#define TAG "4G.c"

void UART_4G_INST()
{
    ESP_ERROR_CHECK(uart_driver_install(UART_4G_NUM, BUF_SIZE, BUF_SIZE, 10, &uart0_event_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_4G_NUM, &uart_config_4G));
    ESP_ERROR_CHECK(uart_set_pin(UART_4G_NUM, UART_4G_TX, UART_4G_RX, -1, -1));
    ESP_LOGI(TAG, "UART_4G has been installed.");
}

void AIR780EP_INST()
{
    char response[BUF_SIZE] = "\0";
    while (1)
    {
    //重启模块
        strcpy(response, SEND_AT_CMD(AT_RESET, 3000)); 
        if( strstr(response, "OK") == NULL) continue;
        ESP_LOGI(TAG, "The module has been reset.");
        
    //检查SIM卡状态
        strcpy(response, SEND_AT_CMD(AT_CPIN, AT_RESPONSE_DELAY)); 
        char *data_pointer = NULL;
        
        if( strstr(response, "READY") == NULL)
        {
            ESP_LOGI(TAG, "Bad SIM Card Status! return");
            continue;
        }
        ESP_LOGI(TAG, "SIM Card is ready.");

    //检查信号强度
        strcpy(response, SEND_AT_CMD(AT_CSQ, AT_RESPONSE_DELAY)); 
        data_pointer = strstr(response, "+CSQ: ") + 6;
        int csq = *(data_pointer + 1) == ','?(*data_pointer - '0') : (*data_pointer - '0')*10 + (*(data_pointer+1) - '0');
        //这一段的目的是将"+CSQ:"后面跟着的数值从字符串里提取出来
        //思路是：寻找"+CSQ:"子串的起始地址，用data_pointer指针指向该地址+6，即数据字符的位置；
        //如果data_pointer+1是逗号，说明数值只有一位；如果data_pointer+1不是逗号，说明数值有两位；分别处理，这里用一个三元
        //众所周知，一个char类型的数字减去一个'0'就是int类型的数字
        
        if( csq > 18 ) ESP_LOGI(TAG, "CSQ is %d, the GSM signal strength is strong.",csq);
        else if( csq > 9 ) ESP_LOGI(TAG, "CSQ is %d, the GSM signal strength is moderate.",csq);
        else{
            ESP_LOGI(TAG, "CSQ is %d, the GSM signal strength is too weak! return.",csq);
            continue;
        }

    //查询网络注册情况
        strcpy(response, SEND_AT_CMD(AT_CGATT, AT_RESPONSE_DELAY)); 
        data_pointer = strstr(response, "+CGATT: ") + 8; //思路同上
        if( *data_pointer == '1') ESP_LOGI(TAG, "The network registration is successful.");
        else{
            ESP_LOGI(TAG, "The network registration is not successful! return.");
            continue;
        }

    //配置数据网络
        strcpy(response, SEND_AT_CMD(AT_CSTT, AT_RESPONSE_DELAY)); 
        if( strstr(response, "OK") == NULL) continue;

    //激活数据网络
        strcpy(response, SEND_AT_CMD(AT_CIICR, AT_RESPONSE_DELAY)); 
        if( strstr(response, "OK") == NULL) continue;

    //查询数据网络是否激活成功
        strcpy(response, SEND_AT_CMD(AT_CIFSR, AT_RESPONSE_DELAY)); 
        if( strstr(response, "ERROR") != NULL) continue;

        break;
    }

    ESP_LOGI(TAG, "Air780EP has been installed.");
    vTaskDelete(NULL);
}


char* SEND_AT_CMD(const char* cmd, int delay)
{
//清除FIFO中可能存在的乱七八糟的缓存
    uart_flush(UART_4G_NUM);

    uart_write_bytes(UART_4G_NUM, cmd, strlen(cmd));
    ESP_LOGI(TAG, "Sent CMD: %s",cmd);
    
    vTaskDelay(pdMS_TO_TICKS(delay));

    static char buffer[BUF_SIZE] = "\0";
    int len = uart_read_bytes(UART_4G_NUM, buffer, BUF_SIZE - 1, 10);

    ESP_LOGI(TAG, "AT Response:");
    printf("%s",buffer);

    if (len > 0) {
        buffer[len] = '\0';
        return buffer;
    } else {
        return "\0";
    }
}


/*
    Transfer SYNC_DATA from ESP32 to EC800M through UART1
    relay_status(1 Byte) + power(6 Bytes) + power_thresh(4 Bytes) + battery_or_dc(1 Byte)
    + battery_voltage_percent(3 Bytes) + WIFI_or_4G(1 Byte) = 16 Bytes
*/
/*int SYNC_DATA_TRANSFER(SYNC_DATA_t DATA)
{
    char data[19];
    sprintf(data, "#%d%-6.1f%-4d%d%-3d%d$",DATA.relay_status,DATA.power,DATA.power_thresh,DATA.battery_or_dc,DATA.battery_voltage_percent,DATA.WIFI_or_4G);
    uart_write_bytes(UART_4G_NUM, data, strlen(data));
    printf("Data Transferred! %s\n",data);
    return 0;
}
*/

void SYNC_DATA_TRANSFER(SYNC_DATA_t DATA)
{
    uart_write_bytes(UART_4G_NUM, AT_MPUB, strlen(AT_MPUB));
    printf("Written: %s\n",AT_MPUB);
    vTaskDelay(pdMS_TO_TICKS(AT_RESPONSE_DELAY));

}

void RELAY_STATUS_UPDATE(int new_status)
{
    if(new_status == RELAY_ON) uart_write_bytes(UART_4G_NUM, AT_MPUB_RELAY_ON, strlen(AT_MPUB_RELAY_ON));
        else uart_write_bytes(UART_4G_NUM, AT_MPUB_RELAY_OFF, strlen(AT_MPUB_RELAY_OFF));
        
    ESP_LOGI(TAG, "New status %d updated to cloud platform.", new_status);
}

void SYNC_DATA_RECEIVE()
{

}

void UART1_EVENT_TASK()
{
    ESP_LOGI(TAG, "UART1_EVENT_TASK has been started.");

    UART_BL0942_INST();

    uart_event_t event;

    while(1)
    {
        if(xQueueReceive(uart1_event_queue, &event, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "Receive event.");
            if(event.type == UART_DATA)
            {
                ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                uint8_t data[event.size];
                uart_read_bytes(UART_4G_NUM, data, event.size, portMAX_DELAY);
                printf("%s",data);
                
                /* char msg_buf[event.size+1];
                memcpy(msg_buf, data, event.size);
                msg_buf[event.size] = '\0';
                printf("%s",msg_buf);
                if(strstr(msg_buf, "+MSUB:") != NULL)
                {
                    printf("msg_buf: %c",msg_buf[42]);
                } */
            }
        }
    }
}
