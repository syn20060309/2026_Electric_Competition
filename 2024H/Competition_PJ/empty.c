#include "ti_msp_dl_config.h"
#include "delay.h"
#include "usart.h"
#include "app_motor_usart.h"
#include "app_motor.h"
#include "app_irtracking.h"
#include "key.h"
#include "led.h"
#include "buzzer.h"


#define MOTOR_TYPE 5   //1:520电机 2:310电机 3:测速码盘TT电机 4:TT直流减速电机 5:L型520电机
                       //1:520 motor 2:310 motor 3:speed code disc TT motor 4:TT DC reduction motor 5:L type 520 motor

int g_LinePortal_flag = 0;
char buffer[80];
int main(void)
{
    USART_Init();//打印串口初始化、四路电机通信串口初始化   Print serial port initialization, four-channel motor communication serial port initialization

    //设置电机类型    Set motor type
    Set_Motor(MOTOR_TYPE);
    send_upload_data(false,false,true);delay_ms(10); //平均速度积分计算里程,详细可以参考四路驱动板协议 The mileage is calculated by integrating the average speed. For detailed information, please refer to the four-way driver board protocol

    //蜂鸣器初始化  Buzzer initialization
    PWM_Buzzer_Init();
    //问题状态机初始化  Problem state machine initialization
    //State_Machine_init();
    
    //清除定时器中断标志 Clear timer interrupt flag
    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
    //使能定时器中断   Enable Timer Interrupt
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    //定时器开始计时   Timer start
    DL_TimerA_startCounter(TIMER_0_INST);
    
    //初始化成功后，LED灯快闪两下，蜂鸣器快响两下   After the initialization is successful, the LED light flashes twice, and the buzzer rings twice
    int i = 0;
    for(i=0;i<4;i++)
    {
        LED2_Toggle();
        Buzzer_Toggle();
        delay_ms(100);
    }
    
	while(1)
	{
        Key_Handle();
        if(g_LinePortal_flag)
        {
            LineWalking();
        }
	}
	
}

