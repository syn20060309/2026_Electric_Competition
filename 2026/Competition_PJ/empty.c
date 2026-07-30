#include "ti_msp_dl_config.h"
#include "delay.h"
#include "usart.h"
#include "app_motor_usart.h"
#include "app_motor.h"
#include "app_irtracking.h"
#include "key.h"
#include "timer.h"
#include "led.h"
#include "buzzer.h"
#include "lap_finish.h"
#include "oled_task.h"
#include "mpu6050_startup.h"


#define MOTOR_TYPE 5   //1:520电机 2:310电机 3:测速码盘TT电机 4:TT直流减速电机 5:L型520电机
                       //1:520 motor 2:310 motor 3:speed code disc TT motor 4:TT DC reduction motor 5:L type 520 motor

int g_LinePortal_flag = 0;

int main(void)
{
    USART_Init();//打印串口初始化、四路电机通信串口初始化   Print serial port initialization, four-channel motor communication serial port initialization

    MPU6050_Startup();

    //设置电机类型    Set motor type
    Contrl_Pwm(0,0,0,0);
    Set_Motor(MOTOR_TYPE);
    Contrl_Pwm(0,0,0,0);

    PWM_Buzzer_Init();
    
    //清除定时器中断标志 Clear timer interrupt flag
    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
    //使能定时器中断   Enable Timer Interrupt
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    //定时器开始计时   Timer start
    DL_TimerA_startCounter(TIMER_0_INST);

    LapFinish_Init();
    OLED_TaskInit();

    int i = 0;
    for(i=0;i<4;i++)
    {
        LED2_Toggle();
        Buzzer_Toggle();
        delay_ms(100);
    }

	while(1)
	{
        uint32_t now = Get_Time();
		Key_Handle();
        OLED_Task(now);
		if(g_LinePortal_flag)
		{
			LineWalking();
		}
	}
}

