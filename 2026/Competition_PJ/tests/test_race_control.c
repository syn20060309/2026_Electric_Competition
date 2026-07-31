#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "car_mode.h"
#include "lap_finish.h"
#include "race_control.h"
#include "race_timer.h"

int g_LinePortal_flag;
static uint32_t fake_time_ms;
static unsigned int stop_command_count;

uint32_t Get_Time(void)
{
    return fake_time_ms;
}

void Contrl_Pwm(
    int16_t m1_pwm, int16_t m2_pwm, int16_t m3_pwm, int16_t m4_pwm)
{
    assert(m1_pwm == 0);
    assert(m2_pwm == 0);
    assert(m3_pwm == 0);
    assert(m4_pwm == 0);
    stop_command_count++;
}

static void reset_fakes(void)
{
    g_LinePortal_flag = 0;
    fake_time_ms = 0U;
    stop_command_count = 0U;
    RaceTimer_Reset();
    LapFinish_Reset();
    CarMode_Init();
}

static void test_none_mode_cannot_start(void)
{
    reset_fakes();

    assert(!Car_StartSelectedMode(500U));

    assert(g_LinePortal_flag == 0);
    assert(RaceTimer_GetState() == RACE_TIMER_IDLE);
    assert(LapFinish_GetState() == LAP_STATE_IDLE);
}

static void test_selected_mode_resets_and_starts_one_race(void)
{
    reset_fakes();
    CarMode_SelectNext();
    fake_time_ms = 500U;

    assert(Car_StartSelectedMode(fake_time_ms));

    assert(g_LinePortal_flag == 1);
    assert(RaceTimer_GetState() == RACE_TIMER_RUNNING);
    assert(RaceTimer_GetElapsedMs() == 0U);
    assert(LapFinish_GetState() == LAP_STATE_LEAVING_START);
}

static void test_finish_stops_motor_timer_and_latches_finish(void)
{
    reset_fakes();
    CarMode_SelectNext();
    assert(Car_StartSelectedMode(0U));
    assert(LapFinish_Update(100U, 100U, true, 0x0FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(200U, 200U, true, 0x0FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(20000U, 20000U, true, 0xFFU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(20001U, 20001U, true, 0x3FU) ==
        LAP_FINISH_EVENT_FINISH);
    fake_time_ms = 20001U;

    Car_FinishStop();

    assert(g_LinePortal_flag == 0);
    assert(stop_command_count == 1U);
    assert(RaceTimer_GetState() == RACE_TIMER_STOPPED);
    assert(RaceTimer_GetElapsedMs() == 20001U);
    assert(LapFinish_GetState() == LAP_STATE_FINISHED);
    assert(CarMode_GetCurrent() == CAR_MODE_QUESTION_2);
}

static void test_abort_is_not_marked_as_finish(void)
{
    reset_fakes();
    CarMode_SelectNext();
    assert(Car_StartSelectedMode(0U));
    fake_time_ms = 2500U;

    Car_AbortStop();

    assert(g_LinePortal_flag == 0);
    assert(stop_command_count == 1U);
    assert(RaceTimer_GetElapsedMs() == 2500U);
    assert(LapFinish_GetState() == LAP_STATE_ABORTED);
    assert(CarMode_GetCurrent() == CAR_MODE_QUESTION_2);
}

static void test_timeout_is_not_marked_as_finish(void)
{
    reset_fakes();
    CarMode_SelectNext();
    CarMode_SelectNext();
    assert(Car_StartSelectedMode(0U));
    assert(LapFinish_CheckTimeout(35000U));
    fake_time_ms = 35000U;

    Car_TimeoutStop();

    assert(g_LinePortal_flag == 0);
    assert(stop_command_count == 1U);
    assert(RaceTimer_GetElapsedMs() == 35000U);
    assert(LapFinish_GetState() == LAP_STATE_TIMEOUT);
    assert(!LapFinish_IsFinished());
    assert(CarMode_GetCurrent() == CAR_MODE_BALL_CONTROL);
}

int main(void)
{
    test_none_mode_cannot_start();
    test_selected_mode_resets_and_starts_one_race();
    test_finish_stops_motor_timer_and_latches_finish();
    test_abort_is_not_marked_as_finish();
    test_timeout_is_not_marked_as_finish();
    return 0;
}
