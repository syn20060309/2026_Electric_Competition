#ifndef	_APP_MPU6050_H_
#define _APP_MPU6050_H_

#include "ti_msp_dl_config.h"
#include "bsp_mpu6050.h"
#include "inv_mpu.h"
#include "delay.h"
#include "timer.h"

extern float pitch,roll,yaw;   //??????? Euler Angles

extern float yawBias,pitchBias,rollBias;
extern float calibratedYaw, calibratedPitch, calibratedRoll;
extern short angle[3];
extern short accel[3];

typedef struct
{
	float Xoffset;
	float Yoffset;
	float Zoffset;
} Bias_t; 


void Get_EulerAngles(void);
float Dir_PID(float error);
float navigetion_0_360_limit(float angle);
float get_minor_arc(float azimuth,float headingAngle);
void AngleOffsetCalc(void);
void Get_CalibratedAngles(void);
void Get_Angle(uint8_t way);

#endif

