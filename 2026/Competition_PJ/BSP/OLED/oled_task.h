#ifndef OLED_TASK_H
#define OLED_TASK_H

#include <stdint.h>

void OLED_TaskInit(void);
void OLED_Task(uint32_t now_ms);

#endif
