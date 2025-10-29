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

typedef struct
{
    bool AT_CMD_SENDING_FLAG;
    char AT_RESPOND[BUF_SIZE];
    SemaphoreHandle_t done;

} at_waiter_t;


QueueHandle_t uart_4G_event_queue = NULL;
static at_waiter_t at_waiter;

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
    //重启模块
    restart_4g:
        memset(response,0,sizeof(response));
        strcpy(response, SEND_AT_CMD(AT_RESET, 3000)); 
        vTaskDelay(pdMS_TO_TICKS(2000));
        if( strstr(response, "OK") == NULL) goto restart_4g;
        ESP_LOGI(TAG, "The module has been reset.");

    while (1)
    {

        for(int i=3;i>=1;i--)
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
            ESP_LOGE(TAG, "Bad SIM Card Status! Returning.");
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
            ESP_LOGE(TAG, "CSQ is %d, the GSM signal strength is too weak! Returning.",csq);
            continue;
        }

    //查询网络注册情况
        memset(response,0,sizeof(response));
        strcpy(response, SEND_AT_CMD(AT_CGATT, AT_RESPONSE_DELAY)); 
        data_pointer = strstr(response, "+CGATT: ") + 8; //思路同上
        if( *data_pointer == '1') ESP_LOGI(TAG, "The network registration is successful.");
        else{
            ESP_LOGE(TAG, "The network registration is not successful! Returning.");
            continue;
        }

    //配置数据网络
        memset(response,0,sizeof(response));
        SEND_AT_CMD(AT_CIPSHUT, AT_RESPONSE_DELAY);
        strcpy(response, SEND_AT_CMD(AT_CSTT, AT_RESPONSE_DELAY)); 
        if( strstr(response, "OK") == NULL)
        {
            ESP_LOGE(TAG, "Data network configuration failed! Returning.");
            continue;
        }
        else ESP_LOGI(TAG, "Data network configuration success.");

    //激活数据网络
        memset(response,0,sizeof(response));
        strcpy(response, SEND_AT_CMD(AT_CIICR, AT_RESPONSE_DELAY)); 
        if( strstr(response, "OK") == NULL) 
        {
            ESP_LOGE(TAG, "Data network activation failed! Returning.");
            continue;
        }

    //查询数据网络是否激活成功
        memset(response,0,sizeof(response));
        strcpy(response, SEND_AT_CMD(AT_CIFSR, AT_RESPONSE_DELAY)); 
        if( strstr(response, "ERROR") != NULL) 
        {
            ESP_LOGE(TAG, "Data network activation failed! Returning.");
            continue;
        }
        else ESP_LOGI(TAG, "Data network activation success.");
    
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
    
    //客户端向服务器请求会话连接
        memset(response,0,sizeof(response));
        strcpy(response, SEND_AT_CMD(AT_MCONNECT, AT_RESPONSE_DELAY));
        if( strstr(response, "OK") == NULL) continue;
    

    // //订阅主题
    //     char cmd[50];    

    //     sprintf(cmd, "AT+MSUB=\"/topic/relay_status_ctrl\",0\r\n");
    //     memset(response,0,sizeof(response));
    //     strcpy(response, SEND_AT_CMD_NO_PRINT(cmd, AT_RESPONSE_DELAY));
    //     if(strstr(response,"OK") != NULL) ESP_LOGI(TAG, "/topic/relay_status_ctrl subscribed.");
    //     else
    //     {
    //         ESP_LOGI(TAG, "topic/relay_status_ctrl subscribe failed!");
    //         continue;
    //     }

    //     sprintf(cmd, "AT+MSUB=\"/topic/power_thresh_ctrl\",0\r\n");
    //     memset(response,0,sizeof(response));
    //     strcpy(response, SEND_AT_CMD_NO_PRINT(cmd, AT_RESPONSE_DELAY));
    //     if(strstr(response,"OK") != NULL) ESP_LOGI(TAG, "/topic/power_thresh_ctrl subscribed.");
    //     else
    //     {
    //         ESP_LOGI(TAG, "topic/power_thresh_ctrl subscribe failed!");
    //         continue;
    //     }


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

//处理接收到的串口数据中的一行
static void handle_one_line(const char* line, const int line_len)
{
    //如果是正在发送AT指令等回复，则交给AT
    if(at_waiter.AT_CMD_SENDING_FLAG == 1){
        //把单行拼接成完整response
        strcat(at_waiter.AT_RESPOND, line);

        //如果发现有OK或ERROR，则说明AT指令的返回完成了
        if( strstr(line, "OK") != NULL || strstr(line, "ERROR") != NULL)
        {
            xSemaphoreGive(at_waiter.done);
            at_waiter.AT_CMD_SENDING_FLAG = 0;
        }

    }
}

/*跟4G模块通信的串口的数据接收任务
    因为4G模块收到的信息有三种可能：
        1. 发送AT指令后的回复
        2. 异步上报（URC）：模块自己发送的信息，比如MQTT订阅消息
        3. 乱七八糟的东西：比如模块重启时会有个"boot"
    处理串口接收到内容的思路是把收到的消息按行分割，逐行处理。
    如果是AT指令的返回，就交给at_waiter.AT_RESPOND，当检测到OK或ERROR时at_waiter.done;
    如果是'+'开头的URC，交给urc_handler();
    
*/
void AIR780EP_RX_TASK()
{
    ESP_LOGI(TAG, "AIR780EP_RX_TASK Starts.");

    at_waiter.AT_CMD_SENDING_FLAG = 0;
    at_waiter.done = xSemaphoreCreateBinary();

    uart_event_t event; //UART事件
    static char buf[BUF_SIZE]; //uart_read_bytes用的buf
    static char line[BUF_SIZE]; //合成一行用的buf
    static int line_len; //当前行的长度

    while(1){

        if(xQueueReceive(uart_4G_event_queue, &event, portMAX_DELAY))
        {
            switch (event.type)
            {
            case UART_DATA:
                //读取ring buffer中数据
                int len = uart_read_bytes(UART_4G_NUM, &buf, event.size, 100/portTICK_PERIOD_MS);
                
                if(len > 0)
                {
                    for(int i = 0; i < len; i++)
                    {
                        //提取每个字符
                        char c = (char)buf[i];
                        if(line_len < BUF_SIZE - 1)
                        {
                            line[line_len] = c;
                            line_len++;
                            line[line_len] = '\0';
                        }

                        //如果发现'\n'字符说明已经收集到一行
                        if(c == '\n')
                        {
                            //ESP_LOGI(TAG, "RX received line: %s", line);
                            //处理单行数据
                            handle_one_line(line, line_len);

                            //清空单行缓存，接收下一行
                            line_len = 0;
                            line[line_len] = '\0';
                        }
                    }
                }

                break;
            
            default:
                break;
            }
            
        }
    }
}

//检测4G联网是否正常
void AIR780EP_LIVE_DAEMON()
{
    ESP_LOGI(TAG, "AIR780EP_LIVE_DAEMON() Started.");

    while(1)
    {
        //如果当前是WIFI上报MQTT，每20秒检查4G是否还活着
        if(MQTT_WIFI_CONNECTED_FLAG == 1)
        {
            ESP_LOGI(TAG, "Air780EP live daemon working...");

            char response[BUF_SIZE] = "\0";
            memset(response,0,sizeof(response));
            strcpy(response, SEND_AT_CMD_NO_PRINT(AT_MQTTSTATU, AT_RESPONSE_DELAY));
            if(strstr(response,"+MQTTSTATU :1") == NULL)
            {
                ESP_LOGE(TAG, "Bad 4G MQTTSTATUS!");
                continue;
            }
            else ESP_LOGI(TAG, "Good 4G MQTTSTATUS.");

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
char* SEND_AT_CMD(const char* cmd, const int delay)
{
    at_waiter.AT_CMD_SENDING_FLAG = 1;
    strcpy(at_waiter.AT_RESPOND, "\0");

    uart_write_bytes(UART_4G_NUM, cmd, strlen(cmd));
    ESP_LOGI(TAG, "Sent CMD: %s",cmd);

    //等待AIR780EP_RX_TASK接收完回复后释放信号量，否则阻塞，最多等1秒
    xSemaphoreTake(at_waiter.done, 1000/portTICK_PERIOD_MS);
    
    ESP_LOGI(TAG, "AT Response:");
    printf("%s",at_waiter.AT_RESPOND);
    at_waiter.AT_CMD_SENDING_FLAG = 0;

    return at_waiter.AT_RESPOND;
}

//发送AT指令但不printf回复
char* SEND_AT_CMD_NO_PRINT(const char* cmd, const int delay)
{
    at_waiter.AT_CMD_SENDING_FLAG = 1;
    strcpy(at_waiter.AT_RESPOND, "\0");

    uart_write_bytes(UART_4G_NUM, cmd, strlen(cmd));
    ESP_LOGI(TAG, "Sent CMD: %s",cmd);

    xSemaphoreTake(at_waiter.done, 1000/portTICK_PERIOD_MS);

    at_waiter.AT_CMD_SENDING_FLAG = 0;

    return at_waiter.AT_RESPOND;

}
