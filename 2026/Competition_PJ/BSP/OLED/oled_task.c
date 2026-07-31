#include "oled_task.h"

#include <stdbool.h>
#include <stdio.h>

#include "BSP/Mode/car_mode.h"
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
static CarMode displayed_mode = CAR_MODE_NONE;
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

static void OLED_ShowAcceleration(uint8_t x_row, uint8_t y_row)
{
    MPU6050_AccelSample sample;

    OLED_ShowString(0U, x_row, OLED_BLANK_LINE);
    OLED_ShowString(0U, y_row, OLED_BLANK_LINE);
    if (!MPU6050_AccelGetLatest(&sample)) {
        OLED_ShowString(0U, x_row, "AX:---");
        OLED_ShowString(0U, y_row, "AY:---");
        return;
    }

    OLED_ShowAccelerationAxis(x_row, "AX", sample.x_g);
    OLED_ShowAccelerationAxis(y_row, "AY", sample.y_g);
}

static void OLED_ShowModeSelection(void)
{
    char buffer[24];

    OLED_Clear();
    OLED_ShowString(0U, OLED_STATUS_ROW, CarMode_GetDisplayName());
    if (CarMode_IsSelected()) {
        (void) snprintf(buffer, sizeof(buffer), "SPD:%u PRESS K1",
            (unsigned int) CarMode_GetSpeed());
        OLED_ShowString(0U, OLED_ACCEL_X_ROW, buffer);
    } else {
        OLED_ShowString(0U, OLED_ACCEL_X_ROW, "HOLD K1");
    }
    OLED_ShowAcceleration(OLED_ACCEL_Y_ROW, OLED_INFO_ROW);
    OLED_Update();
}

static void OLED_ShowStatusLabel(const char *status)
{
    char buffer[24];
    const char *mode_name = CarMode_GetShortName();

    if (mode_name[0] == '\0') {
        OLED_ShowString(0U, OLED_STATUS_ROW, status);
        return;
    }

    (void) snprintf(buffer, sizeof(buffer), "%s %s", status, mode_name);
    OLED_ShowString(0U, OLED_STATUS_ROW, buffer);
}

static void OLED_ShowRaceState(
    RaceTimerState state, LapFinish_State lap_state)
{
    if (state == RACE_TIMER_IDLE) {
        OLED_ShowModeSelection();
        return;
    }

    OLED_Clear();
    if (lap_state == LAP_STATE_FINISHED) {
        OLED_ShowStatusLabel("FINISH");
    } else if (lap_state == LAP_STATE_ABORTED) {
        OLED_ShowStatusLabel("ABORT");
    } else if (lap_state == LAP_STATE_TIMEOUT) {
        OLED_ShowStatusLabel("TIMEOUT");
    } else if (state == RACE_TIMER_RUNNING) {
        OLED_ShowStatusLabel("RUN");
    } else if (state == RACE_TIMER_STOPPED) {
        OLED_ShowStatusLabel("STOP");
    } else {
        OLED_ShowStatusLabel("IDLE");
    }

    OLED_ShowAcceleration(OLED_ACCEL_X_ROW, OLED_ACCEL_Y_ROW);
    OLED_ShowTime(RaceTimer_GetElapsedMs());
    OLED_Update();
}

void OLED_TaskInit(void)
{
    oled_task_ready = false;
    displayed_state = RACE_TIMER_IDLE;
    displayed_lap_state = LAP_STATE_IDLE;
    displayed_mode = CarMode_GetCurrent();
    last_oled_update_ms = 0U;

    OLED_Init();
    if (!OLED_IsReady()) {
        return;
    }

    oled_task_ready = true;
    OLED_ShowModeSelection();
}

void OLED_Task(uint32_t now_ms)
{
    RaceTimerState state;
    LapFinish_State lap_state;
    CarMode mode;

    if (!oled_task_ready || !OLED_IsReady()) {
        oled_task_ready = false;
        return;
    }

    state = RaceTimer_GetState();
    lap_state = LapFinish_GetState();
    mode = CarMode_GetCurrent();
    if ((state != displayed_state) ||
        (lap_state != displayed_lap_state) ||
        (mode != displayed_mode)) {
        displayed_state = state;
        displayed_lap_state = lap_state;
        displayed_mode = mode;
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
