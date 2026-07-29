#ifndef RACE_CONTROL_H
#define RACE_CONTROL_H

#include <stdint.h>

void Car_RaceStart(uint32_t now_ms);
void Car_FinishStop(void);
void Car_AbortStop(void);
void Car_TimeoutStop(void);

#endif
