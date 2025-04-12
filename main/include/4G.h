/*
    File: 4G.h
    Memo: 控制和4G模块的通信
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#ifndef __4G_H__
#define __4G_H__

#define AT_RESPONSE_DELAY 1000

//Air780EP AT CMD
#define AT_CIMI "AT+CIMI\r\n"
#define AT_RESET "AT+RESET\r\n" //重启模块
#define AT_CPIN "AT+CPIN?\r\n" //查询SIM卡是否准备好
#define AT_CSQ "AT+CSQ\r\n" //查询信号强度
#define AT_CGATT "AT+CGATT?\r\n" //查询上网服务是否激活
#define AT_CSTT "AT+CSTT\r\n" //配置数据网络
#define AT_CIICR "AT+CIICR\r\n" //激活数据网络
#define AT_CIFSR "AT+CIFSR\r\n" //查询数据网络是否激活成功


typedef struct{
    bool relay_status; //1: relay is conductive; 0: relay is not conductive.
    float power; // at most 5 significant figure (xxxx.x Watt)
    int power_thresh; //at most 4 significant figure (xxxx Watt)
    bool battery_or_dc; //1: battery mode; 0: dc mode.
    int battery_voltage_percent; //at most 3 significant figure (100% to 0%)
    bool WIFI_or_4G; //1: WIFI mode; 0: 4G mode.
} SYNC_DATA_t;

void AIR780EP_INST(); //初始化4G模块
char* SEND_AT_CMD(const char* cmd, int delay); //发送AT指令并返回串口的回复内容
void MQTT_INST(); //MQTT服务初始化
void SYNC_DATA_TRANSFER(SYNC_DATA_t DATA);
void SYNC_DATA_RECEIVE();
void RELAY_STATUS_UPDATE();
void UART1_EVENT_TASK();

#endif