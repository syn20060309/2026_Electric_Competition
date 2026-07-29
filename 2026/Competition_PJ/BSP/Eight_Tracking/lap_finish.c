#include "lap_finish.h"

#include "tracking_sample.h"

static LapFinish_State lap_state = LAP_STATE_IDLE;
static bool clear_candidate_active;
static uint32_t clear_candidate_start_ms;
static bool start_line_cleared;
static uint8_t finish_confirm_count;

void LapFinish_Init(void)
{
    LapFinish_Reset();
}

void LapFinish_Reset(void)
{
    lap_state = LAP_STATE_IDLE;
    clear_candidate_active = false;
    clear_candidate_start_ms = 0U;
    start_line_cleared = false;
    finish_confirm_count = 0U;
}

void LapFinish_Start(uint32_t now_ms)
{
    lap_state = LAP_STATE_LEAVING_START;
    clear_candidate_active = false;
    clear_candidate_start_ms = now_ms;
    start_line_cleared = false;
    finish_confirm_count = 0U;
}

bool LapFinish_CheckTimeout(uint32_t elapsed_ms)
{
    if (((lap_state == LAP_STATE_LEAVING_START) ||
            (lap_state == LAP_STATE_RUNNING)) &&
        (elapsed_ms >= LAP_SAFETY_TIMEOUT_MS)) {
        lap_state = LAP_STATE_TIMEOUT;
        return true;
    }

    return false;
}

LapFinish_Event LapFinish_Update(uint32_t now_ms, uint32_t elapsed_ms,
    bool sensor_data_valid, uint8_t active_mask)
{
    uint8_t active_count;

    if ((lap_state != LAP_STATE_LEAVING_START) &&
        (lap_state != LAP_STATE_RUNNING)) {
        return LAP_FINISH_EVENT_NONE;
    }

    if (!sensor_data_valid) {
        if (lap_state == LAP_STATE_LEAVING_START) {
            clear_candidate_active = false;
        }
        finish_confirm_count = 0U;
        return LAP_FINISH_EVENT_NONE;
    }

    active_count = Tracking_CountActive(active_mask);

    if (lap_state == LAP_STATE_LEAVING_START) {
        finish_confirm_count = 0U;

        if (active_count >= FINISH_ACTIVE_COUNT_THRESHOLD) {
            clear_candidate_active = false;
            return LAP_FINISH_EVENT_NONE;
        }

        if (!clear_candidate_active) {
            clear_candidate_active = true;
            clear_candidate_start_ms = now_ms;
        } else if ((uint32_t) (now_ms - clear_candidate_start_ms) >=
            START_LINE_CLEAR_CONFIRM_MS) {
            start_line_cleared = true;
            lap_state = LAP_STATE_RUNNING;
        }

        return LAP_FINISH_EVENT_NONE;
    }

    if (!start_line_cleared || (elapsed_ms < FINISH_MIN_TIME_MS)) {
        finish_confirm_count = 0U;
        return LAP_FINISH_EVENT_NONE;
    }

    if (active_count < FINISH_ACTIVE_COUNT_THRESHOLD) {
        finish_confirm_count = 0U;
        return LAP_FINISH_EVENT_NONE;
    }

    if (finish_confirm_count < FINISH_CONFIRM_SAMPLE_COUNT) {
        finish_confirm_count++;
    }

    if (finish_confirm_count >= FINISH_CONFIRM_SAMPLE_COUNT) {
        lap_state = LAP_STATE_FINISH_DETECTED;
        return LAP_FINISH_EVENT_FINISH;
    }

    return LAP_FINISH_EVENT_NONE;
}

LapFinish_State LapFinish_GetState(void)
{
    return lap_state;
}

void LapFinish_MarkFinished(void)
{
    if (lap_state == LAP_STATE_FINISH_DETECTED) {
        lap_state = LAP_STATE_FINISHED;
    }
}

void LapFinish_MarkAborted(void)
{
    if ((lap_state == LAP_STATE_LEAVING_START) ||
        (lap_state == LAP_STATE_RUNNING) ||
        (lap_state == LAP_STATE_FINISH_DETECTED)) {
        lap_state = LAP_STATE_ABORTED;
    }
}

void LapFinish_MarkTimeout(void)
{
    if ((lap_state == LAP_STATE_LEAVING_START) ||
        (lap_state == LAP_STATE_RUNNING)) {
        lap_state = LAP_STATE_TIMEOUT;
    }
}

bool LapFinish_IsFinished(void)
{
    return lap_state == LAP_STATE_FINISHED;
}

bool LapFinish_IsAborted(void)
{
    return lap_state == LAP_STATE_ABORTED;
}

bool LapFinish_StartLineCleared(void)
{
    return start_line_cleared;
}

uint8_t LapFinish_GetConfirmCount(void)
{
    return finish_confirm_count;
}
