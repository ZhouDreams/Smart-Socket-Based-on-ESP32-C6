/*
    File: 4G.h
    Memo: 控制和4G模块的通信
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#ifndef __4G_H__
#define __4G_H__

#define AT_RESPONSE_DELAY 300

//Air780EP AT INIT CMD
#define AT_CIMI "AT+CIMI\r\n"
#define AT_RESET "AT+RESET\r\n" //重启模块
#define AT_CPIN "AT+CPIN?\r\n" //查询SIM卡是否准备好
#define AT_CSQ "AT+CSQ\r\n" //查询信号强度
#define AT_CGATT "AT+CGATT?\r\n" //查询上网服务是否激活
#define AT_CSTT "AT+CSTT\r\n" //配置数据网络
#define AT_CIICR "AT+CIICR\r\n" //激活数据网络
#define AT_CIFSR "AT+CIFSR\r\n" //查询数据网络是否激活成功

//Air780EP AT MQTT CMD
#define AT_MCONFIG "AT+MCONFIG=\"Smart_Socket_4G\",\"zhoudreams\",\"sbzjx250\",0,0,\"/topic/online\",\"0\"\r\n" //设置 MQTT 相关参数
#define AT_MQTTMSGSET_1 "AT+MQTTMSGSET=1\r\n" //设置MQTT订阅消息为缓存模式
#define AT_MIPSTART "AT+MIPSTART=\"mqtt.jovisdreams.site\",1883\r\n" //建立 TCP 连接
#define AT_MCONNECT "AT+MCONNECT=1,5\r\n" //客户端向服务器请求会话连接
#define AT_MQTTSTATU "AT+MQTTSTATU\r\n" //查询MQTT连接状态

void UART_4G_INST(); //4G UART初始化
void AIR780EP_INST(); //初始化4G模块
void AIR780EP_LIVE_DAEMON();
char* SEND_AT_CMD(const char* cmd, int delay); //发送AT指令并返回串口的回复内容
char* SEND_AT_CMD_NO_PRINT(const char* cmd, int delay);



#endif