#include "race_timer.h"

extern uint32_t Get_Time(void);

static uint32_t start_time_ms;
static uint32_t elapsed_time_ms;
static RaceTimerState timer_state = RACE_TIMER_IDLE;

void RaceTimer_Start(void)
{
    start_time_ms = Get_Time();
    elapsed_time_ms = 0U;
    timer_state = RACE_TIMER_RUNNING;
}

void RaceTimer_Stop(void)
{
    if (timer_state != RACE_TIMER_RUNNING) {
        return;
    }

    elapsed_time_ms = Get_Time() - start_time_ms;
    timer_state = RACE_TIMER_STOPPED;
}

void RaceTimer_Reset(void)
{
    start_time_ms = 0U;
    elapsed_time_ms = 0U;
    timer_state = RACE_TIMER_IDLE;
}

uint32_t RaceTimer_GetElapsedMs(void)
{
    if (timer_state == RACE_TIMER_RUNNING) {
        return Get_Time() - start_time_ms;
    }

    return elapsed_time_ms;
}

bool RaceTimer_IsRunning(void)
{
    return timer_state == RACE_TIMER_RUNNING;
}

RaceTimerState RaceTimer_GetState(void)
{
    return timer_state;
}
