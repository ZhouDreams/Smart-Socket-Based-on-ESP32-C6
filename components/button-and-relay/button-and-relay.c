/*
    File: button-and-relay.c
    Memo: 按钮和继电器模块
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "button-and-relay.h"

#define TAG "button-and-relay"

static RelayTargetLevel_t s_relay_level;
static QueueHandle_t s_relay_queue;

//GPIO中断服务函数
static void IRAM_ATTR button_isr_handler(void* arg)
{
    RelayCMD_t relay_cmd_buf = {
        .relay_op_source = SCR_BUTTON,
        .relay_op_type = TOGGLE,
        .op_tick = xTaskGetTickCount()
    };
    xQueueSendFromISR(s_relay_queue, &relay_cmd_buf, pdFALSE);
}

//按钮GPIO初始化
void button_gpio_inst()
{
    gpio_set_direction(GPIO_BUTTON_NUM, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_BUTTON_NUM, GPIO_PULLDOWN_ONLY);
    gpio_set_intr_type(GPIO_BUTTON_NUM, GPIO_INTR_POSEDGE); //设置中断为上升沿触发
    gpio_install_isr_service(0); //安装GPIO ISR服务
    gpio_isr_handler_add(GPIO_BUTTON_NUM, button_isr_handler, NULL); //添加按钮的GPIO中断
    ESP_LOGI(TAG, "Button has been installed.");
}

//继电器GPIO初始化
void relay_gpio_inst()
{
    gpio_reset_pin(GPIO_RELAY_NUM);
    gpio_set_direction(GPIO_RELAY_NUM, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_RELAY_NUM, RELAY_OFF);
    ESP_LOGI(TAG, "Relay has been installed.");
}

//向继电器发送命令
void relay_send_cmd(RelayCMD_t relay_cmd)
{
    xQueueSend(s_relay_queue, &relay_cmd, portMAX_DELAY);
}

//获取当前继电器状态
RelayTargetLevel_t relay_get_level()
{
    return s_relay_level;
}

//设置继电器GPIO输出
static void relay_set_level(RelayTargetLevel_t level)
{
    gpio_set_level(GPIO_RELAY_NUM, level);
}

//继电器任务
static void relay_task()
{
    RelayCMD_t relay_cmd_buf;
    uint32_t last_event_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    while (1) {
        if(xQueueReceive(s_relay_queue, &relay_cmd_buf, portMAX_DELAY)) {
            if (relay_cmd_buf.op_tick * portTICK_PERIOD_MS - last_event_time < MAX_OP_INTERVAL_MS) {
                continue;
            }
            switch (relay_cmd_buf.relay_op_type) {
            case TOGGLE:
                s_relay_level = s_relay_level == RELAY_ON? RELAY_OFF:RELAY_ON;
                relay_set_level(s_relay_level);
                break;
            case SET:
                s_relay_level = relay_cmd_buf.relay_target_level;
                relay_set_level(s_relay_level);
            default:
                break;
            }
            last_event_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        }
    }
    

}

//启动继电器任务
void relay_task_start()
{
    s_relay_queue = xQueueCreate(10, sizeof(RelayCMD_t));
    xTaskCreate(relay_task, "relay_task", 4096, NULL, 10, NULL);
    ESP_LOGI(TAG, "relay_task has been started.");
}
