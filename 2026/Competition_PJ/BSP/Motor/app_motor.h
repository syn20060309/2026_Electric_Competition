#ifndef __APP_MOTOR_H_
#define __APP_MOTOR_H_

#include "ti_msp_dl_config.h"
#include "app_motor_usart.h"
#include "delay.h"

// Half of the sum of the motor spacing between the chassis
#define Car_APB (188.0f)

void Set_Motor(int MOTOR_TYPE);
void Motion_Car_Control(int16_t V_x, int16_t V_y, int16_t V_z);
void Get_Odometry(void);

extern uint8_t encoder_odometry_flag;
extern float odometry_sum;

#endif
