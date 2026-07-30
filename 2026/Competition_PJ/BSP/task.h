#ifndef TASK_H
#define TASK_H

#include <stdint.h>

typedef struct {
    uint32_t interval;
    uint32_t last_call;
    void (*task)(void);
} Task;

void Scheduler_Init(void);
void Scheduler_Run(void);

#endif
