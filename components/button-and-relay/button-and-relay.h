/*
    File: button-and-relay.h
    Memo: 按钮和继电器模块
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h" // IWYU pragma: keep

#define GPIO0_PIN 6 //继电器GPIO0
#define GPIO1_PIN 7 //按钮GPIO1
#define GPIO_RELAY_NUM GPIO_NUM_0
#define GPIO_BUTTON_NUM GPIO_NUM_1
#define MAX_OP_INTERVAL_MS 200

typedef enum { TOGGLE, SET } RelayOpType_t;
typedef enum { SRC_BUTTON, SRC_MQTT, SRC_LTE4G } RelayOpSource_t;
typedef enum { RELAY_OFF = 0, RELAY_ON = 1 } RelayTargetLevel_t;

typedef struct 
{
    RelayOpType_t relay_op_type;
    RelayOpSource_t relay_op_source;
    RelayTargetLevel_t relay_target_level;
    TickType_t op_tick;
} RelayCMD_t;

esp_err_t button_gpio_inst(); 

esp_err_t relay_gpio_inst(); 

void relay_task_start(int priority);

void relay_send_cmd(RelayCMD_t relay_cmd);

void relay_send_cmd_from_isr(RelayCMD_t relay_cmd);

RelayTargetLevel_t relay_get_level();

#ifdef __cplusplus
}
#endif

