#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "lap_finish.h"

static void confirm_start_line_cleared(uint32_t first_clear_ms)
{
    assert(LapFinish_Update(first_clear_ms, 100U, true, 0x1FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_GetState() == LAP_STATE_LEAVING_START);

    assert(LapFinish_Update(first_clear_ms + 99U, 199U, true, 0x0FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_GetState() == LAP_STATE_LEAVING_START);

    assert(LapFinish_Update(first_clear_ms + 100U, 200U, true, 0x1FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_GetState() == LAP_STATE_RUNNING);
    assert(LapFinish_StartLineCleared());
}

static void test_initial_state_and_start(void)
{
    LapFinish_Init();
    assert(LapFinish_GetState() == LAP_STATE_IDLE);
    assert(!LapFinish_StartLineCleared());

    LapFinish_Start(1000U);
    assert(LapFinish_GetState() == LAP_STATE_LEAVING_START);
    assert(!LapFinish_StartLineCleared());
}

static void test_start_line_requires_valid_continuous_clear(void)
{
    LapFinish_Reset();
    LapFinish_Start(1000U);

    assert(LapFinish_Update(1010U, 10U, true, 0xFFU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(1020U, 20U, true, 0x1FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(1119U, 119U, false, 0x00U) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(1120U, 120U, true, 0x1FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(1219U, 219U, true, 0x3FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_GetState() == LAP_STATE_LEAVING_START);
    assert(LapFinish_Update(1220U, 220U, true, 0x1FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(1319U, 319U, true, 0x1FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_GetState() == LAP_STATE_LEAVING_START);
    assert(LapFinish_Update(1320U, 320U, true, 0x1FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_GetState() == LAP_STATE_RUNNING);
}

static void test_finish_requires_time_validity_and_two_consecutive_samples(void)
{
    const uint32_t threshold = FINISH_MIN_TIME_MS;

    LapFinish_Reset();
    LapFinish_Start(0U);
    confirm_start_line_cleared(100U);

    assert(LapFinish_Update(threshold - 1U, threshold - 1U, true, 0xFFU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(threshold, threshold, true, 0x1FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(threshold + 1U, threshold + 1U, true, 0x3FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_GetConfirmCount() == 1U);
    assert(LapFinish_Update(threshold + 2U, threshold + 2U, false, 0xFFU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_GetConfirmCount() == 0U);
    assert(LapFinish_Update(threshold + 3U, threshold + 3U, true, 0x3FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_GetConfirmCount() == 1U);
    assert(LapFinish_Update(threshold + 4U, threshold + 4U, true, 0x1FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_GetConfirmCount() == 0U);
    assert(LapFinish_Update(threshold + 5U, threshold + 5U, true, 0xFFU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_GetConfirmCount() == 1U);
    assert(LapFinish_GetState() == LAP_STATE_RUNNING);

    assert(LapFinish_Update(threshold + 6U, threshold + 6U, true, 0x7FU) ==
        LAP_FINISH_EVENT_FINISH);
    assert(LapFinish_GetConfirmCount() == FINISH_CONFIRM_SAMPLE_COUNT);
    assert(LapFinish_GetState() == LAP_STATE_FINISH_DETECTED);
    assert(LapFinish_Update(threshold + 7U, threshold + 7U, true, 0xFFU) ==
        LAP_FINISH_EVENT_NONE);

    LapFinish_MarkFinished();
    assert(LapFinish_GetState() == LAP_STATE_FINISHED);
    assert(LapFinish_IsFinished());
}

static void assert_active_count_can_finish(uint8_t active_mask)
{
    const uint32_t threshold = FINISH_MIN_TIME_MS;

    LapFinish_Reset();
    LapFinish_Start(0U);
    confirm_start_line_cleared(100U);

    assert(LapFinish_Update(threshold, threshold, true, active_mask) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(threshold + 1U, threshold + 1U, true, active_mask) ==
        LAP_FINISH_EVENT_FINISH);
}

static void test_six_seven_and_eight_active_can_finish(void)
{
    assert_active_count_can_finish(0x3FU);
    assert_active_count_can_finish(0x7FU);
    assert_active_count_can_finish(0xFFU);
}

static void test_start_line_must_be_cleared_before_finish(void)
{
    LapFinish_Reset();
    LapFinish_Start(0U);

    assert(LapFinish_Update(20000U, 20000U, true, 0xFFU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(20001U, 20001U, true, 0xFFU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_GetState() == LAP_STATE_LEAVING_START);
    assert(!LapFinish_StartLineCleared());
}

static void test_timeout_is_not_finish(void)
{
    LapFinish_Reset();
    LapFinish_Start(0U);
    confirm_start_line_cleared(100U);

    assert(!LapFinish_CheckTimeout(34999U));
    assert(LapFinish_Update(35000U, 35000U, false, 0xFFU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_GetState() == LAP_STATE_RUNNING);
    assert(LapFinish_CheckTimeout(35000U));
    assert(LapFinish_GetState() == LAP_STATE_TIMEOUT);
    assert(!LapFinish_IsFinished());
}

static void test_abort_and_restart(void)
{
    LapFinish_Reset();
    LapFinish_Start(0U);
    LapFinish_MarkAborted();
    assert(LapFinish_GetState() == LAP_STATE_ABORTED);
    assert(LapFinish_IsAborted());

    LapFinish_Reset();
    LapFinish_Start(500U);
    assert(LapFinish_GetState() == LAP_STATE_LEAVING_START);
    assert(!LapFinish_IsAborted());
}

static void test_clear_confirmation_handles_clock_wrap(void)
{
    LapFinish_Reset();
    LapFinish_Start(UINT32_MAX - 80U);

    assert(LapFinish_Update(UINT32_MAX - 50U, 30U, true, 0x3FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(UINT32_MAX - 40U, 40U, true, 0x1FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(58U, 139U, true, 0x1FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_GetState() == LAP_STATE_LEAVING_START);
    assert(LapFinish_Update(59U, 140U, true, 0x1FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_GetState() == LAP_STATE_RUNNING);
}

int main(void)
{
    test_initial_state_and_start();
    test_start_line_requires_valid_continuous_clear();
    test_finish_requires_time_validity_and_two_consecutive_samples();
    test_six_seven_and_eight_active_can_finish();
    test_start_line_must_be_cleared_before_finish();
    test_timeout_is_not_finish();
    test_abort_and_restart();
    test_clear_confirmation_handles_clock_wrap();
    return 0;
}
