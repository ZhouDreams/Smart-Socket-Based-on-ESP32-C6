/*
    File: button.h
    Memo: 控制按钮
    Coder: Junxi Zhou, School of Microelectronics, South China University of Technology
    Email: zhoudreamstk@foxmail.com
*/

void BUTTON_GPIO3_INST(); //按钮GPIO3初始化
void BUTTON_TASK(); //按钮任务
void BUTTON_ISR_HANDLER(void* arg); //按钮GPIO中断服务函数



