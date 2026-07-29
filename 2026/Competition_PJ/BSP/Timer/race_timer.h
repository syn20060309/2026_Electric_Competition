#ifndef RACE_TIMER_H
#define RACE_TIMER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    RACE_TIMER_IDLE = 0,
    RACE_TIMER_RUNNING,
    RACE_TIMER_STOPPED
} RaceTimerState;

void RaceTimer_Start(void);
void RaceTimer_Stop(void);
void RaceTimer_Reset(void);
uint32_t RaceTimer_GetElapsedMs(void);
bool RaceTimer_IsRunning(void);
RaceTimerState RaceTimer_GetState(void);

#endif
