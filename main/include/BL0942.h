/*
    File: BL0942.h
    Memo: 计量模块
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

#ifndef __BL0942_H__
#define __BL0942_H__

#define BL0942_READ_CMD 0b01011000 //读取命令字节
#define BL0942_IRMS_ADDR 0x03 //电流有效值寄存器
#define BL0942_VRMS_ADDR 0x04 //电压有效值寄存器
#define BL0942_I_FAST_RMS_ADDR 0x05 //电流快速有效值寄存器
#define BL0942_WATT_ADDR 0x06 //有功功率寄存器
#define BL0942_CF_CNT_ADDR 0x07 //有功电能脉冲计数寄存器
#define BL0942_I_FAST_RMS_TH_ADDR 0x15 //电流快速有效值阈值寄存器（过流保护）
#define BL0942_READ_ALL 0xAA //全电参数数据包

#define BL0942_WRITE_CMD 0b10101000

void UART_BL0942_INST(); //BL0942 UART初始化
void BL0942_READ_TASK();



#endif


