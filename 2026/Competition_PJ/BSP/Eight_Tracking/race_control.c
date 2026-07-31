#include "race_control.h"

#include <stdint.h>

#include "BSP/Mode/car_mode.h"
#include "lap_finish.h"
#include "race_timer.h"

extern int g_LinePortal_flag;
extern void Contrl_Pwm(
    int16_t m1_pwm, int16_t m2_pwm, int16_t m3_pwm, int16_t m4_pwm);

static void Car_Stop(void)
{
    g_LinePortal_flag = 0;
    Contrl_Pwm(0, 0, 0, 0);
    RaceTimer_Stop();
}

bool Car_StartSelectedMode(uint32_t now_ms)
{
    if (!CarMode_IsSelected()) {
        return false;
    }

    LapFinish_Reset();
    RaceTimer_Start();
    LapFinish_Start(now_ms);
    g_LinePortal_flag = 1;
    return true;
}

void Car_FinishStop(void)
{
    Car_Stop();
    LapFinish_MarkFinished();
}

void Car_AbortStop(void)
{
    Car_Stop();
    LapFinish_MarkAborted();
}

void Car_TimeoutStop(void)
{
    Car_Stop();
    LapFinish_MarkTimeout();
}
