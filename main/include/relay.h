/*
    File: relay.h
    Memo: 操作继电器用的函数
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#ifndef __RELAY_H__
#define __RELAY_H__

void RELAY_GPIO2_INST(); //继电器GPIO2初始化
void RELAY_SWITCH_ON();
void RELAY_SWITCH_OFF();
void MQTT_SWITCH_RELAY();
void MQTT_SET_RELAY(bool value);
void RELAY_TASK(); //继电器事件处理程序
void relay_test(); 

#endif