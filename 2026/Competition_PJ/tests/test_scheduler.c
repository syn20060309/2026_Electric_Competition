#include <assert.h>
#include <stdint.h>

#include "task.h"

static uint32_t fake_time_ms;
static unsigned int key_calls;
static unsigned int accel_calls;
static unsigned int oled_calls;
static unsigned int odometry_calls;

uint32_t Get_Time(void)
{
    return fake_time_ms;
}

void Key_Handle(void)
{
    key_calls++;
}

void MPU6050_AccelTask(void)
{
    accel_calls++;
}

void OLED_RefreshTask(void)
{
    oled_calls++;
}

void Get_Odometry(void)
{
    odometry_calls++;
}

static void reset_fakes(uint32_t start_ms)
{
    fake_time_ms = start_ms;
    key_calls = 0U;
    accel_calls = 0U;
    oled_calls = 0U;
    odometry_calls = 0U;
    Scheduler_Init();
}

static void test_tasks_run_at_their_configured_intervals(void)
{
    reset_fakes(0U);

    for (fake_time_ms = 1U; fake_time_ms <= 1000U; fake_time_ms++) {
        Scheduler_Run();
    }

    assert(key_calls == 100U);
    assert(accel_calls == 33U);
    assert(oled_calls == 10U);
    assert(odometry_calls == 1U);
}

static void test_scheduler_runs_each_overdue_task_only_once(void)
{
    reset_fakes(0U);
    fake_time_ms = 5000U;

    Scheduler_Run();

    assert(key_calls == 1U);
    assert(accel_calls == 1U);
    assert(oled_calls == 1U);
    assert(odometry_calls == 1U);
}

static void test_unsigned_time_difference_handles_clock_wrap(void)
{
    reset_fakes(UINT32_MAX - 5U);
    fake_time_ms = 4U;

    Scheduler_Run();

    assert(key_calls == 1U);
    assert(accel_calls == 0U);
    assert(oled_calls == 0U);
    assert(odometry_calls == 0U);
}

int main(void)
{
    test_tasks_run_at_their_configured_intervals();
    test_scheduler_runs_each_overdue_task_only_once();
    test_unsigned_time_difference_handles_clock_wrap();
    return 0;
}
