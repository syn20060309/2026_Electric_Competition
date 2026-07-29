#include "oled_task.h"

#include <stdbool.h>
#include <stdio.h>

#include "oled.h"
#include "race_timer.h"

#define OLED_REFRESH_INTERVAL_MS 100U
#define OLED_STATUS_ROW 0U
#define OLED_PROMPT_ROW 2U
#define OLED_TIME_ROW 4U
#define OLED_BLANK_LINE "                     "

static bool oled_task_ready;
static RaceTimerState displayed_state = RACE_TIMER_IDLE;
static uint32_t last_oled_update_ms;

void OLED_ShowTime(uint32_t elapsed_ms)
{
    char buffer[24];
    uint32_t seconds = elapsed_ms / 1000U;
    uint32_t centiseconds = (elapsed_ms % 1000U) / 10U;

    (void) snprintf(buffer, sizeof(buffer), "TIME:%02lu.%02lus",
        (unsigned long) seconds, (unsigned long) centiseconds);
    OLED_ShowString(0U, OLED_TIME_ROW, buffer);
}

static void OLED_ShowIdleScreen(void)
{
    OLED_Clear();
    OLED_ShowString(0U, OLED_STATUS_ROW, "LINE CAR");
    OLED_ShowString(0U, OLED_PROMPT_ROW, "PRESS KEY");
    OLED_ShowTime(0U);
    OLED_Update();
}

static void OLED_ShowRaceState(RaceTimerState state)
{
    OLED_ShowString(0U, OLED_STATUS_ROW, OLED_BLANK_LINE);
    OLED_ShowString(0U, OLED_PROMPT_ROW, OLED_BLANK_LINE);

    if (state == RACE_TIMER_RUNNING) {
        OLED_ShowString(0U, OLED_STATUS_ROW, "RUNNING");
    } else if (state == RACE_TIMER_STOPPED) {
        OLED_ShowString(0U, OLED_STATUS_ROW, "STOPPED");
    }

    OLED_ShowTime(RaceTimer_GetElapsedMs());
    OLED_Update();
}

void OLED_TaskInit(void)
{
    oled_task_ready = false;
    displayed_state = RACE_TIMER_IDLE;
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

    if (!oled_task_ready || !OLED_IsReady()) {
        oled_task_ready = false;
        return;
    }

    state = RaceTimer_GetState();
    if (state != displayed_state) {
        displayed_state = state;
        last_oled_update_ms = now_ms;
        OLED_ShowRaceState(state);
        return;
    }

    if ((state == RACE_TIMER_RUNNING) &&
        ((uint32_t) (now_ms - last_oled_update_ms) >=
            OLED_REFRESH_INTERVAL_MS)) {
        last_oled_update_ms = now_ms;
        OLED_ShowTime(RaceTimer_GetElapsedMs());
        OLED_Update();
    }
}
