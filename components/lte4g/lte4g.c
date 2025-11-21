/*
    File: 4G.c
    Memo: 控制和4G模块的通信
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include "freertos/FreeRTOS.h" // IWYU pragma: keep 
#include "driver/uart.h"
#include "driver/gpio.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "esp_log.h"
#include "string.h"
#include "lte4g.h"
#include "button-and-relay.h" // IWYU pragma: keep
#include "app-mqtt.h"

static const char* TAG = "lte4g";

typedef struct
{
    bool at_cmd_sending_flag;
    char at_wait_response[BUF_SIZE];
    char at_error_response[BUF_SIZE];
    char at_respond[BUF_SIZE];
    SemaphoreHandle_t done;
    bool error_occurred;

} at_waiter_t;

typedef struct 
{
    bool RESET;
    bool CPIN;
    bool CSQ;
    bool CGATT;
    bool CSTT;
    bool CIFSR;
    bool MCONFIG;
    bool MIPSTART;
    bool MCONNECT;
} lte4g_init_task_t;

static uart_config_t s_lte4g_uart_cfg = {
    .baud_rate = UART_4G_BAUD_RATE,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_DEFAULT,
};

QueueHandle_t s_lte4g_uart_event_queue;
static at_waiter_t s_at_waiter;
static EventGroupHandle_t s_lte4g_online_event;

//4G模块串口驱动安装
static void lte4g_uart_inst()
{
    ESP_ERROR_CHECK(uart_driver_install(UART_4G_NUM, BUF_SIZE, BUF_SIZE, 10, &s_lte4g_uart_event_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_4G_NUM, &s_lte4g_uart_cfg));
    ESP_ERROR_CHECK(uart_set_pin(UART_4G_NUM, UART_4G_TX, UART_4G_RX, -1, -1));
    ESP_LOGI(TAG, "UART_4G has been installed.");
}

/*处理接收到的串口数据中的一行
因为4G模块收到的信息有三种可能：
        1. 发送AT指令后的回复
        2. 异步上报（URC）：模块自己发送的信息，比如MQTT订阅消息
        3. 乱七八糟的东西：比如模块重启时会有个"boot"
    处理串口接收到内容的思路是把收到的消息按行分割，逐行处理。
    如果是AT指令的返回，就交给s_at_waiter.at_respond，当检测到OK或ERROR时s_at_waiter.done;
    如果是+MSUB开头，就交给appmqtt
*/
static void handle_one_line(const char* line, const int line_len)
{
    //如果信息中有+MSUB，说明是MQTT订阅消息
    if(strstr(line, "+MSUB:") != NULL)
    {
        ESP_LOGI(TAG, "Received MSUB: %s", line);
        appmqtt_lte4g_msub_handler(line);
    }
    //如果是正在发送AT指令等回复，则交给AT
    else if(s_at_waiter.at_cmd_sending_flag == 1)
    {
        //把单行拼接成完整response
        strcat(s_at_waiter.at_respond, line);

        //如果发现包含at_wait_response，说明响应完成
        if( strstr(line, s_at_waiter.at_wait_response) != NULL)
        {
            xSemaphoreGive(s_at_waiter.done);
        }
        //如果发现包含at_error_response，说明出错了
        else if (strstr(line, s_at_waiter.at_error_response) != NULL)
        {
            s_at_waiter.error_occurred = 1;
            xSemaphoreGive(s_at_waiter.done);
        }
        

    }
}

//跟4G模块通信的串口的数据接收任务
static void lte4g_rx_task()
{
    ESP_LOGI(TAG, "lte4g_rx_task Starts.");

    s_at_waiter.at_cmd_sending_flag = 0;
    s_at_waiter.done = xSemaphoreCreateBinary();

    uart_event_t event; //UART事件
    static char buf[BUF_SIZE]; //uart_read_bytes用的buf
    static char line[BUF_SIZE]; //合成一行用的buf
    static int line_len; //当前行的长度

    while(1){

        if(xQueueReceive(s_lte4g_uart_event_queue, &event, portMAX_DELAY))
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
                            ESP_LOGI(TAG, "RX received line: %s", line);
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

static void lte4g_rx_task_start(int priority)
{
     xTaskCreate(lte4g_rx_task, "lte4g_rx_task", 4096, NULL, priority, NULL);
}

//4G模块初始化
static void lte4g_module_init_task()
{
    s_lte4g_online_event = xEventGroupCreate();
    xEventGroupSetBits(s_lte4g_online_event, LTE4G_OFFLINE);

    gpio_reset_pin(LTE4G_EN_GPIO);
    gpio_set_direction(LTE4G_EN_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LTE4G_EN_GPIO, 0);

    lte4g_uart_inst();
    lte4g_rx_task_start(10);
    lte4g_init_task_t lte4g_init_task = {0,0,0,0,0,0,0,0,0};

    char response[BUF_SIZE] = "\0";
    //重启模块
        // memset(response,0,sizeof(response));
        // strcpy(response, lte4g_send_at_cmd(AT_RESET)); 
        // vTaskDelay(pdMS_TO_TICKS(2000));
        // if( strstr(response, "OK") == NULL) goto restart_4g;
        // lte4g_init_task.RESET = 1;
        // ESP_LOGI(TAG, "The module has been reset.");

        gpio_set_level(LTE4G_EN_GPIO, 1);

        for(int i=3;i>=1;i--)
        {
            ESP_LOGI(TAG, "Starting 4G INIT in %ds...",i);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

    while (1)
    {
        ESP_LOGI(TAG, "Trying to INIT 4G...");
        vTaskDelay(pdMS_TO_TICKS(1000));

        //检查SIM卡状态
        if(lte4g_init_task.CPIN == 0){
            memset(response,0,sizeof(response));
            strcpy(response, lte4g_send_at_cmd(AT_CPIN, "OK", "ERROR", AT_WAIT_TICKS_NORMAL)); 
            char *data_pointer = NULL;
        
            if( strstr(response, "READY") == NULL)
            {
                ESP_LOGE(TAG, "Bad SIM Card Status! Returning.");
                continue;
            }
            lte4g_init_task.CPIN = 1;
            ESP_LOGI(TAG, "SIM Card is ready.");
        }

        //检查信号强度
        if(lte4g_init_task.CSQ == 0){
            memset(response,0,sizeof(response));
            strcpy(response, lte4g_send_at_cmd(AT_CSQ, "OK", "ERROR", AT_WAIT_TICKS_NORMAL)); 
            char *data_pointer = NULL;
            data_pointer = strstr(response, "+CSQ: ") + 6;
            int csq = *(data_pointer + 1) == ','?(*data_pointer - '0') : (*data_pointer - '0')*10 + (*(data_pointer+1) - '0');
            //这一段的目的是将"+CSQ:"后面跟着的数值从字符串里提取出来
            //思路是：寻找"+CSQ:"子串的起始地址，用data_pointer指针指向该地址+6，即数据字符的位置；
            //如果data_pointer+1是逗号，说明数值只有一位；如果data_pointer+1不是逗号，说明数值有两位；分别处理，这里用一个三元
            //众所周知，一个char类型的数字减去一个'0'就是int类型的数字
            if( csq == 99 ){
                ESP_LOGE(TAG, "CSQ is %d, bad GSM signal! Returning.",csq);
                continue;
            }
            else if( csq > 18 ) ESP_LOGI(TAG, "CSQ is %d, the GSM signal strength is strong.",csq);
            else if( csq > 9 ) ESP_LOGI(TAG, "CSQ is %d, the GSM signal strength is moderate.",csq);
            else{
                ESP_LOGE(TAG, "CSQ is %d, the GSM signal strength is too weak! Returning.",csq);
                continue;
            }

            lte4g_init_task.CSQ = 1;
        }

        //查询网络注册情况
        if(lte4g_init_task.CGATT == 0){
            memset(response,0,sizeof(response));
            strcpy(response, lte4g_send_at_cmd(AT_CGATT, "OK", "ERROR", AT_WAIT_TICKS_NORMAL)); 
            char *data_pointer = NULL;
            data_pointer = strstr(response, "+CGATT: ");
            if (data_pointer == NULL ){
                ESP_LOGE(TAG, "The network registration is not successful! Returning.");
                continue;
            }
            data_pointer += 8; //思路同上
            if( *data_pointer == '1') ESP_LOGI(TAG, "The network registration is successful.");
            else{
                ESP_LOGE(TAG, "The network registration is not successful! Returning.");
                continue;
            }

            lte4g_init_task.CGATT = 1;
        }

        //配置数据网络
        if(lte4g_init_task.CSTT == 0){
            memset(response,0,sizeof(response));
            lte4g_send_at_cmd(AT_CIPSHUT, "OK", "ERROR", AT_WAIT_TICKS_NORMAL);
            strcpy(response, lte4g_send_at_cmd(AT_CSTT, "OK", "ERROR", AT_WAIT_TICKS_NORMAL)); 
            if( strstr(response, "OK") == NULL)
            {
                ESP_LOGE(TAG, "Data network configuration failed! Returning.");
                continue;
            }
            else ESP_LOGI(TAG, "Data network configuration success.");

            lte4g_init_task.CSTT = 1;
        }

        //激活数据网络
        if(lte4g_init_task.CIFSR == 0){
            memset(response,0,sizeof(response));
            strcpy(response, lte4g_send_at_cmd(AT_CIICR, "OK", "ERROR", AT_WAIT_TICKS_NORMAL)); 
            if( strstr(response, "OK") == NULL) 
            {
                ESP_LOGE(TAG, "Data network activation failed! Returning.");
                continue;
            }

        //查询数据网络是否激活成功
            memset(response,0,sizeof(response));
            strcpy(response, lte4g_send_at_cmd(AT_CIFSR, ".", "ERROR", AT_WAIT_TICKS_NORMAL)); 
            if( strstr(response, "ERROR") != NULL) 
            {
                ESP_LOGE(TAG, "Data network activation failed! Returning.");
                continue;
            }
            else ESP_LOGI(TAG, "Data network activation success.");

            lte4g_init_task.CIFSR = 1;
        }

        break;
    }
    xEventGroupClearBits(s_lte4g_online_event, LTE4G_OFFLINE);
    xEventGroupSetBits(s_lte4g_online_event, LTE4G_ONLINE);
    ESP_LOGI(TAG, "online event.");
    // xTaskCreate(AIR780EP_LIVE_DAEMON, "AIR780EP_LIVE_DAEMON", 4096, NULL, 1, NULL);
    vTaskDelete(NULL);
}

EventGroupHandle_t lte4g_get_online_event()
{
    return s_lte4g_online_event;
}

void lte4g_module_init_task_start(int priority)
{
    xTaskCreate(lte4g_module_init_task, "lte4g_software_inst_task", 4096, NULL, priority, NULL);
}

//检测4G联网是否正常
// void AIR780EP_LIVE_DAEMON()
// {
//     ESP_LOGI(TAG, "AIR780EP_LIVE_DAEMON() Started.");

//     while(1)
//     {
//         //如果当前是WIFI上报MQTT，每20秒检查4G是否还活着
//         if(MQTT_WIFI_CONNECTED_FLAG == 1)
//         {
//             ESP_LOGI(TAG, "Air780EP live daemon working...");

//             char response[BUF_SIZE] = "\0";
//             memset(response,0,sizeof(response));
//             strcpy(response, lte4g_send_at_no_print(AT_MQTTSTATU));
//             if(strstr(response,"+MQTTSTATU :1") == NULL)
//             {
//                 ESP_LOGE(TAG, "Bad 4G MQTTSTATUS!");
//                 continue;
//             }
//             else ESP_LOGI(TAG, "Good 4G MQTTSTATUS.");

//             vTaskDelay(pdMS_TO_TICKS(20000));
//         }
//         else vTaskDelay(pdMS_TO_TICKS(20000));
//         //如果当前是4G上报MQTT,负责接收订阅信息
//         // else if(Air780EP_ONLINE_FLAG == 1)
//         // {
//         //     static char buffer[BUF_SIZE] = "\0";
//         //     memset(buffer,0,sizeof(buffer));
//         //     uart_event_t event;
//         //     int len = 0;

//         //     // if(xQueueReceive(s_lte4g_uart_event_queue, (void *)&event, (TickType_t)portMAX_DELAY))
//         //     // {
//         //     //     len = uart_read_bytes(UART_4G_NUM, buffer, BUF_SIZE - 1, 10);
//         //     // }

//         //     // if(strstr(buffer,"+MSUB: \"/topic/relay_status\",1 byte,") != NULL)
//         //     // {
//         //     //     printf("len = %d",len);
//         //     // }
//         // }
//     }
// }

//发送AT指令并printf回复
char* lte4g_send_at_cmd(const char* cmd, const char* wait_str, const char* error_str,TickType_t wait_time_ticks)
{
    s_at_waiter.at_cmd_sending_flag = 1;
    s_at_waiter.error_occurred = 0;
    strcpy(s_at_waiter.at_respond, "\0");
    strcpy(s_at_waiter.at_wait_response, wait_str);
    strcpy(s_at_waiter.at_error_response, error_str);

    uart_write_bytes(UART_4G_NUM, cmd, strlen(cmd));
    ESP_LOGI(TAG, "Sent CMD: %s",cmd);

    //等待lte4g_rx_task接收完回复后释放信号量，否则阻塞
    xSemaphoreTake(s_at_waiter.done, wait_time_ticks);
    
    ESP_LOGI(TAG, "AT Response:");
    printf("%s",s_at_waiter.at_respond);
    s_at_waiter.at_cmd_sending_flag = 0;

    return s_at_waiter.at_respond;
}

//发送AT指令但不printf回复
char* lte4g_send_at_no_print(const char* cmd, const char* wait_str, const char* error_str, TickType_t wait_time_ticks)
{
    s_at_waiter.at_cmd_sending_flag = 1;
    s_at_waiter.error_occurred = 0;
    strcpy(s_at_waiter.at_respond, "\0");
    strcpy(s_at_waiter.at_wait_response, wait_str);
    strcpy(s_at_waiter.at_error_response, error_str);

    uart_write_bytes(UART_4G_NUM, cmd, strlen(cmd));
    xSemaphoreTake(s_at_waiter.done, wait_time_ticks);

    s_at_waiter.at_cmd_sending_flag = 0;

    return s_at_waiter.at_respond;
}
