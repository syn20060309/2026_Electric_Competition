#include "lap_finish.h"

static LapFinish_State lap_state = LAP_STATE_IDLE;
static bool clear_candidate_active;
static uint32_t clear_candidate_start_ms;
static bool start_line_cleared;

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
}

void LapFinish_Start(uint32_t now_ms)
{
    lap_state = LAP_STATE_LEAVING_START;
    clear_candidate_active = false;
    clear_candidate_start_ms = now_ms;
    start_line_cleared = false;
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
    if ((lap_state != LAP_STATE_LEAVING_START) &&
        (lap_state != LAP_STATE_RUNNING)) {
        return LAP_FINISH_EVENT_NONE;
    }

    if (!sensor_data_valid) {
        if (lap_state == LAP_STATE_LEAVING_START) {
            clear_candidate_active = false;
        }
        return LAP_FINISH_EVENT_NONE;
    }

    if (lap_state == LAP_STATE_LEAVING_START) {
        if (active_mask == FINISH_ALL_ACTIVE_MASK) {
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

    if (start_line_cleared &&
        (elapsed_ms >= FINISH_MIN_TIME_MS) &&
        (active_mask == FINISH_ALL_ACTIVE_MASK)) {
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
