#ifndef RACE_CONTROL_H
#define RACE_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

bool Car_StartSelectedMode(uint32_t now_ms);
void Car_FinishStop(void);
void Car_AbortStop(void);
void Car_TimeoutStop(void);

#endif
