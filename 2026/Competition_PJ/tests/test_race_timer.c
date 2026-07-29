#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "race_timer.h"

static uint32_t fake_time_ms;

uint32_t Get_Time(void)
{
    return fake_time_ms;
}

static void test_power_on_is_idle(void)
{
    RaceTimer_Reset();

    assert(RaceTimer_GetState() == RACE_TIMER_IDLE);
    assert(!RaceTimer_IsRunning());
    assert(RaceTimer_GetElapsedMs() == 0U);
}

static void test_stop_while_idle_has_no_effect(void)
{
    RaceTimer_Reset();
    fake_time_ms = 500U;

    RaceTimer_Stop();

    assert(RaceTimer_GetState() == RACE_TIMER_IDLE);
    assert(RaceTimer_GetElapsedMs() == 0U);
}

static void test_running_time_uses_millisecond_difference(void)
{
    RaceTimer_Reset();
    fake_time_ms = 1000U;
    RaceTimer_Start();
    fake_time_ms = 1837U;

    assert(RaceTimer_GetState() == RACE_TIMER_RUNNING);
    assert(RaceTimer_IsRunning());
    assert(RaceTimer_GetElapsedMs() == 837U);
}

static void test_stop_freezes_elapsed_time(void)
{
    RaceTimer_Reset();
    fake_time_ms = 200U;
    RaceTimer_Start();
    fake_time_ms = 1434U;
    RaceTimer_Stop();
    fake_time_ms = 9000U;

    assert(RaceTimer_GetState() == RACE_TIMER_STOPPED);
    assert(!RaceTimer_IsRunning());
    assert(RaceTimer_GetElapsedMs() == 1234U);
}

static void test_restart_resets_elapsed_time(void)
{
    RaceTimer_Reset();
    fake_time_ms = 10U;
    RaceTimer_Start();
    fake_time_ms = 510U;
    RaceTimer_Stop();
    fake_time_ms = 1000U;
    RaceTimer_Start();

    assert(RaceTimer_GetState() == RACE_TIMER_RUNNING);
    assert(RaceTimer_GetElapsedMs() == 0U);
}

static void test_elapsed_time_survives_tick_wraparound(void)
{
    RaceTimer_Reset();
    fake_time_ms = UINT32_MAX - 5U;
    RaceTimer_Start();
    fake_time_ms = 4U;

    assert(RaceTimer_GetElapsedMs() == 10U);
}

int main(void)
{
    test_power_on_is_idle();
    test_stop_while_idle_has_no_effect();
    test_running_time_uses_millisecond_difference();
    test_stop_freezes_elapsed_time();
    test_restart_resets_elapsed_time();
    test_elapsed_time_survives_tick_wraparound();
    return 0;
}
