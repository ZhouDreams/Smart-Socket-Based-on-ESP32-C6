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

QueueHandle_t uart_4G_event_queue = NULL;
int AT_CMD_SENDING_FLAG = 0;

//4G模块串口驱动安装
void UART_4G_INST()
{
    ESP_ERROR_CHECK(uart_driver_install(UART_4G_NUM, BUF_SIZE, BUF_SIZE, 10, &uart_4G_event_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_4G_NUM, &uart_config_4G));
    ESP_ERROR_CHECK(uart_set_pin(UART_4G_NUM, UART_4G_TX, UART_4G_RX, -1, -1));
    ESP_LOGI(TAG, "UART_4G has been installed.");
}

//4G模块初始化
void AIR780EP_INST()
{
    char response[BUF_SIZE] = "\0";
    while (1)
    {

    //重启模块
        memset(response,0,sizeof(response));
        strcpy(response, SEND_AT_CMD(AT_RESET, 3000)); 
        vTaskDelay(pdMS_TO_TICKS(2000));
        if( strstr(response, "OK") == NULL) continue;
        ESP_LOGI(TAG, "The module has been reset.");

        for(int i=5;i>=1;i--)
        {
            ESP_LOGI(TAG, "Starting 4G INIT in %ds...",i);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        
    //检查SIM卡状态
        memset(response,0,sizeof(response));
        strcpy(response, SEND_AT_CMD(AT_CPIN, AT_RESPONSE_DELAY)); 
        char *data_pointer = NULL;
        
        if( strstr(response, "READY") == NULL)
        {
            ESP_LOGE(TAG, "Bad SIM Card Status! return");
            continue;
        }
        ESP_LOGI(TAG, "SIM Card is ready.");

    //检查信号强度
        memset(response,0,sizeof(response));
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
            ESP_LOGE(TAG, "CSQ is %d, the GSM signal strength is too weak! return.",csq);
            continue;
        }

    //查询网络注册情况
        memset(response,0,sizeof(response));
        strcpy(response, SEND_AT_CMD(AT_CGATT, AT_RESPONSE_DELAY)); 
        data_pointer = strstr(response, "+CGATT: ") + 8; //思路同上
        if( *data_pointer == '1') ESP_LOGI(TAG, "The network registration is successful.");
        else{
            ESP_LOGE(TAG, "The network registration is not successful! return.");
            continue;
        }

    //配置数据网络
        memset(response,0,sizeof(response));
        strcpy(response, SEND_AT_CMD(AT_CSTT, AT_RESPONSE_DELAY)); 
        if( strstr(response, "OK") == NULL) continue;

    //激活数据网络
        memset(response,0,sizeof(response));
        strcpy(response, SEND_AT_CMD(AT_CIICR, AT_RESPONSE_DELAY)); 
        if( strstr(response, "OK") == NULL) continue;

    //查询数据网络是否激活成功
        memset(response,0,sizeof(response));
        strcpy(response, SEND_AT_CMD(AT_CIFSR, AT_RESPONSE_DELAY)); 
        if( strstr(response, "ERROR") != NULL) continue;
    
    //设置MQTT相关参数
        memset(response,0,sizeof(response));
        strcpy(response, SEND_AT_CMD(AT_MCONFIG, AT_RESPONSE_DELAY));
        if( strstr(response, "OK") == NULL) continue;

    //设置MQTT订阅消息为缓存模式
        memset(response,0,sizeof(response));
        strcpy(response, SEND_AT_CMD(AT_MQTTMSGSET_1, AT_RESPONSE_DELAY));
        if( strstr(response, "OK") == NULL) continue;
    
    //建立TCP连接
        memset(response,0,sizeof(response));
        strcpy(response, SEND_AT_CMD(AT_MIPSTART, AT_RESPONSE_DELAY));
        if( strstr(response, "OK") == NULL) continue;
    
    //这里一定要delay一下，否则MCONNECT会在TCP建立连接前就发出，导致会话失败。
        vTaskDelay(pdMS_TO_TICKS(500)); 
    
    //客户端向服务器请求会话连接
        memset(response,0,sizeof(response));
        strcpy(response, SEND_AT_CMD(AT_MCONNECT, AT_RESPONSE_DELAY));
        if( strstr(response, "OK") == NULL) continue;
    
    //同理
        vTaskDelay(pdMS_TO_TICKS(500)); 

    //订阅主题
        char cmd[50];    

        sprintf(cmd, "AT+MSUB=\"/topic/relay_status_ctrl\",0\r\n");
        memset(response,0,sizeof(response));
        strcpy(response, SEND_AT_CMD_NO_PRINT(cmd, AT_RESPONSE_DELAY));
        if(strstr(response,"OK") != NULL) ESP_LOGI(TAG, "/topic/relay_status_ctrl subscribed.");
        else
        {
            ESP_LOGI(TAG, "topic/relay_status_ctrl subscribe failed!");
            continue;
        }


        // sprintf(cmd, "AT+MSUB=\"/topic/relay_status\",0\r\n");
        // memset(response,0,sizeof(response));
        // strcpy(response, SEND_AT_CMD_NO_PRINT(cmd, AT_RESPONSE_DELAY));
        // if(strstr(response,"OK") != NULL) ESP_LOGI(TAG, "/topic/relay_status subscribed.");
        // else
        // {
        //     ESP_LOGI(TAG, "topic/relay_status subscribe failed!");
        //     continue;
        // }

        // sprintf(cmd, "AT+MSUB=\"/topic/network\",0\r\n");
        // memset(response,0,sizeof(response));
        // strcpy(response, SEND_AT_CMD_NO_PRINT(cmd, AT_RESPONSE_DELAY));
        // if(strstr(response,"OK") != NULL) ESP_LOGI(TAG, "/topic/network subscribed.");
        // else
        // {
        //     ESP_LOGI(TAG, "topic/network subscribe failed!");
        //     continue;
        // }

        // sprintf(cmd, "AT+MSUB=\"/topic/power_thresh\",0\r\n");
        // memset(response,0,sizeof(response));
        // strcpy(response, SEND_AT_CMD_NO_PRINT(cmd, AT_RESPONSE_DELAY));
        // if(strstr(response,"OK") != NULL) ESP_LOGI(TAG, "/topic/power_thresh subscribed.");
        // else
        // {
        //     ESP_LOGI(TAG, "topic/power_thresh subscribe failed!");
        //     continue;
        // }

        break;
    }

    Air780EP_ONLINE_FLAG = 1;
    ESP_LOGI(TAG, "MQTT 4G has been activated.");
    xTaskCreate(AIR780EP_LIVE_DAEMON, "AIR780EP_LIVE_DAEMON", 4096, NULL, 1, NULL);
    vTaskDelete(NULL);
}

//4G模块保活进程
void AIR780EP_LIVE_DAEMON()
{
    ESP_LOGI(TAG, "AIR780EP_LIVE_DAEMON() Started.");

    while(1)
    {
        //如果当前是WIFI上报MQTT，每20秒检查4G是否还活着
        if(MQTT_WIFI_CONNECTED_FLAG == 1)
        {
            ESP_LOGI(TAG, "AIR780EP live daemon working...");

            char response[BUF_SIZE] = "\0";
            memset(response,0,sizeof(response));
            strcpy(response, SEND_AT_CMD_NO_PRINT(AT_MQTTSTATU, AT_RESPONSE_DELAY));
            if(strstr(response,"+MQTTSTATU :1") == NULL)
            {
                ESP_LOGE(TAG, "Bad 4G MQTTSTATU!");
                continue;
            }
            else ESP_LOGI(TAG, "Good 4G MQTTSTATU.");

            vTaskDelay(pdMS_TO_TICKS(20000));
        }
        else vTaskDelay(pdMS_TO_TICKS(20000));
        //如果当前是4G上报MQTT,负责接收订阅信息
        // else if(Air780EP_ONLINE_FLAG == 1)
        // {
        //     static char buffer[BUF_SIZE] = "\0";
        //     memset(buffer,0,sizeof(buffer));
        //     uart_event_t event;
        //     int len = 0;

        //     // if(xQueueReceive(uart_4G_event_queue, (void *)&event, (TickType_t)portMAX_DELAY))
        //     // {
        //     //     len = uart_read_bytes(UART_4G_NUM, buffer, BUF_SIZE - 1, 10);
        //     // }

        //     // if(strstr(buffer,"+MSUB: \"/topic/relay_status\",1 byte,") != NULL)
        //     // {
        //     //     printf("len = %d",len);
        //     // }
        // }
    }
}


//发送AT指令并printf回复
char* SEND_AT_CMD(const char* cmd, int delay)
{
//清除FIFO中可能存在的乱七八糟的缓存
    uart_flush(UART_4G_NUM);
    AT_CMD_SENDING_FLAG = 1;

    static char buffer[BUF_SIZE] = "\0";
    memset(buffer,0,sizeof(buffer));
    uart_event_t event;
    int len = 0;

    char cmd_no_LF[strlen(cmd)];
    strcpy(cmd_no_LF,cmd);
    cmd_no_LF[strlen(cmd)-1] = '\0';

    uart_write_bytes(UART_4G_NUM, cmd, strlen(cmd));
    ESP_LOGI(TAG, "Sent CMD: %s",cmd_no_LF);
    
    //vTaskDelay(pdMS_TO_TICKS(delay));

    if (xQueueReceive(uart_4G_event_queue, (void *)&event, (TickType_t)portMAX_DELAY))
    {
        if(event.type == UART_DATA)
        {
            len = uart_read_bytes(UART_4G_NUM, buffer, BUF_SIZE - 1, 10);
            
        }
    }
    
    ESP_LOGI(TAG, "AT Response:");
    printf("%s",buffer);

    if (len > 0) {
        buffer[len] = '\0';
        return buffer;
    } else {
        return "\0";
    }

    AT_CMD_SENDING_FLAG = 0;
}

//发送AT指令但不printf回复
char* SEND_AT_CMD_NO_PRINT(const char* cmd, int delay)
{
//清除FIFO中可能存在的乱七八糟的缓存
    uart_flush(UART_4G_NUM);
    AT_CMD_SENDING_FLAG = 1;

    uart_write_bytes(UART_4G_NUM, cmd, strlen(cmd));
    char cmd_no_LF[strlen(cmd)];
    strcpy(cmd_no_LF,cmd);
    cmd_no_LF[strlen(cmd)-1] = '\0';
    ESP_LOGI(TAG, "Sent CMD: %s",cmd_no_LF);
    
    //vTaskDelay(pdMS_TO_TICKS(delay));

    static char buffer[BUF_SIZE] = "\0";
    memset(buffer,0,sizeof(buffer));
    int len = 0;

    for(;;)
    {
        uart_event_t event;
        if (xQueueReceive(uart_4G_event_queue, (void *)&event, (TickType_t)portMAX_DELAY))
        {
            if(event.type == UART_DATA)
            {
                len = uart_read_bytes(UART_4G_NUM, buffer, BUF_SIZE - 1, 10);
                break;
            }
        }
    }

    // ESP_LOGI(TAG, "AT Response:");
    // printf("%s",buffer);

    if (len > 0) {
        buffer[len] = '\0';
        return buffer;
    } else {
        return "\0";
    }

    AT_CMD_SENDING_FLAG = 0;
}
