#include "task.h"

#include <stddef.h>

#include "acceleration_task.h"
#include "app_motor.h"
#include "key.h"
#include "oled_task.h"
#include "timer.h"

static Task tasks[] = {
    {10U, 0U, Key_Handle},
    {30U, 0U, MPU6050_AccelTask},
    {100U, 0U, OLED_RefreshTask},
    {1000U, 0U, Get_Odometry},
};

static size_t Task_Count(void)
{
    return sizeof(tasks) / sizeof(tasks[0]);
}

void Scheduler_Init(void)
{
    const uint32_t now = Get_Time();
    size_t i;

    for (i = 0U; i < Task_Count(); i++) {
        tasks[i].last_call = now;
    }
}

void Scheduler_Run(void)
{
    const uint32_t now = Get_Time();
    size_t i;

    for (i = 0U; i < Task_Count(); i++) {
        if ((uint32_t) (now - tasks[i].last_call) >= tasks[i].interval) {
            tasks[i].task();
            tasks[i].last_call = now;
        }
    }
}
