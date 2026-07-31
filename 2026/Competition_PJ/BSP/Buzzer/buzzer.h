#ifndef	__BUZZER_H__
#define __BUZZER_H__

#include "ti_msp_dl_config.h"
#include "delay.h"
#include "led.h"

#define BUZZER_INIT_ON_MS       (60U)
#define BUZZER_INIT_GAP_MS      (60U)
#define BUZZER_Q2_ON_MS         (120U)
#define BUZZER_BALL_ON_MS       (120U)
#define BUZZER_BALL_GAP_MS      (250U)

extern volatile uint32_t bee_time;           //蜂鸣器鸣叫的时间（单位：ms） the sound_time of beep(the unit is ms)

void Buzzer_Handle(void);
void PWM_Buzzer_Init(void);
void Buzzer_ON(void);
void Buzzer_OFF(void);
void Beep_Times(int times);
void Buzzer_Toggle(void);
void Buzzer_NotifyInitComplete(void);
void Buzzer_NotifyQ2Mode(void);
void Buzzer_NotifyBallMode(void);

#endif
