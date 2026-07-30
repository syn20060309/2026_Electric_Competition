#include "oled_task.h"

#include <stdbool.h>
#include <stdio.h>

#include "acceleration_task.h"
#include "lap_finish.h"
#include "oled.h"
#include "race_timer.h"

extern uint32_t Get_Time(void);

#define OLED_REFRESH_INTERVAL_MS 100U
#define OLED_STATUS_ROW 0U
#define OLED_ACCEL_X_ROW 2U
#define OLED_ACCEL_Y_ROW 4U
#define OLED_INFO_ROW 6U
#define OLED_BLANK_LINE "                     "

static bool oled_task_ready;
static RaceTimerState displayed_state = RACE_TIMER_IDLE;
static LapFinish_State displayed_lap_state = LAP_STATE_IDLE;
static uint32_t last_oled_update_ms;

void OLED_ShowTime(uint32_t elapsed_ms)
{
    char buffer[24];
    uint32_t seconds = elapsed_ms / 1000U;
    uint32_t centiseconds = (elapsed_ms % 1000U) / 10U;

    (void) snprintf(buffer, sizeof(buffer), "TIME:%02lu.%02lus",
        (unsigned long) seconds, (unsigned long) centiseconds);
    OLED_ShowString(0U, OLED_INFO_ROW, buffer);
}

static int32_t OLED_ToMilliG(float acceleration_g)
{
    float scaled = acceleration_g * 1000.0f;

    return (scaled >= 0.0f)
        ? (int32_t) (scaled + 0.5f)
        : (int32_t) (scaled - 0.5f);
}

static void OLED_ShowAccelerationAxis(
    uint8_t row, const char *axis, float acceleration_g)
{
    char buffer[24];
    int32_t milli_g = OLED_ToMilliG(acceleration_g);
    char sign = '+';
    uint32_t magnitude;

    if (milli_g < 0) {
        sign = '-';
        magnitude = (uint32_t) (-milli_g);
    } else {
        magnitude = (uint32_t) milli_g;
    }

    (void) snprintf(buffer, sizeof(buffer), "%s:%c%lu.%03lug",
        axis, sign, (unsigned long) (magnitude / 1000U),
        (unsigned long) (magnitude % 1000U));
    OLED_ShowString(0U, row, buffer);
}

static void OLED_ShowAcceleration(void)
{
    MPU6050_AccelSample sample;

    OLED_ShowString(0U, OLED_ACCEL_X_ROW, OLED_BLANK_LINE);
    OLED_ShowString(0U, OLED_ACCEL_Y_ROW, OLED_BLANK_LINE);
    if (!MPU6050_AccelGetLatest(&sample)) {
        OLED_ShowString(0U, OLED_ACCEL_X_ROW, "AX:---");
        OLED_ShowString(0U, OLED_ACCEL_Y_ROW, "AY:---");
        return;
    }

    OLED_ShowAccelerationAxis(OLED_ACCEL_X_ROW, "AX", sample.x_g);
    OLED_ShowAccelerationAxis(OLED_ACCEL_Y_ROW, "AY", sample.y_g);
}

static void OLED_ShowIdleScreen(void)
{
    OLED_Clear();
    OLED_ShowString(0U, OLED_STATUS_ROW, "IDLE");
    OLED_ShowAcceleration();
    OLED_ShowString(0U, OLED_INFO_ROW, "PRESS KEY");
    OLED_Update();
}

static void OLED_ShowRaceState(
    RaceTimerState state, LapFinish_State lap_state)
{
    OLED_ShowString(0U, OLED_STATUS_ROW, OLED_BLANK_LINE);
    OLED_ShowString(0U, OLED_INFO_ROW, OLED_BLANK_LINE);

    if (lap_state == LAP_STATE_FINISHED) {
        OLED_ShowString(0U, OLED_STATUS_ROW, "FINISH");
    } else if (lap_state == LAP_STATE_ABORTED) {
        OLED_ShowString(0U, OLED_STATUS_ROW, "ABORT");
    } else if (lap_state == LAP_STATE_TIMEOUT) {
        OLED_ShowString(0U, OLED_STATUS_ROW, "TIMEOUT");
    } else if (state == RACE_TIMER_RUNNING) {
        OLED_ShowString(0U, OLED_STATUS_ROW, "RUNNING");
    } else if (state == RACE_TIMER_STOPPED) {
        OLED_ShowString(0U, OLED_STATUS_ROW, "STOPPED");
    } else {
        OLED_ShowString(0U, OLED_STATUS_ROW, "IDLE");
    }

    OLED_ShowAcceleration();
    if (state == RACE_TIMER_IDLE) {
        OLED_ShowString(0U, OLED_INFO_ROW, "PRESS KEY");
    } else {
        OLED_ShowTime(RaceTimer_GetElapsedMs());
    }
    OLED_Update();
}

void OLED_TaskInit(void)
{
    oled_task_ready = false;
    displayed_state = RACE_TIMER_IDLE;
    displayed_lap_state = LAP_STATE_IDLE;
    last_oled_update_ms = 0U;

    OLED_Init();
    if (!OLED_IsReady()) {
        return;
    }

    oled_task_ready = true;
    OLED_ShowIdleScreen();
}

void OLED_Task(uint32_t now_ms)
{
    RaceTimerState state;
    LapFinish_State lap_state;

    if (!oled_task_ready || !OLED_IsReady()) {
        oled_task_ready = false;
        return;
    }

    state = RaceTimer_GetState();
    lap_state = LapFinish_GetState();
    if ((state != displayed_state) ||
        (lap_state != displayed_lap_state)) {
        displayed_state = state;
        displayed_lap_state = lap_state;
        last_oled_update_ms = now_ms;
        OLED_ShowRaceState(state, lap_state);
        return;
    }

    if ((uint32_t) (now_ms - last_oled_update_ms) >=
        OLED_REFRESH_INTERVAL_MS) {
        last_oled_update_ms = now_ms;
        OLED_ShowRaceState(state, lap_state);
    }
}

void OLED_RefreshTask(void)
{
    OLED_Task(Get_Time());
}
