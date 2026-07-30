#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "oled.h"
#include "oled_task.h"
#include "lap_finish.h"
#include "race_timer.h"

static uint32_t fake_time_ms;
static bool fake_oled_ready;
static unsigned int update_count;
static char rendered_lines[8][24];

uint32_t Get_Time(void)
{
    return fake_time_ms;
}

void OLED_Init(void)
{
}

bool OLED_IsReady(void)
{
    return fake_oled_ready;
}

void OLED_Clear(void)
{
    memset(rendered_lines, 0, sizeof(rendered_lines));
}

void OLED_Update(void)
{
    update_count++;
}

void OLED_ShowChar(uint8_t x, uint8_t y, char ch)
{
    if ((y < 8U) && (x < (sizeof(rendered_lines[0]) - 1U))) {
        rendered_lines[y][x] = ch;
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, const char *str)
{
    if ((y < 8U) && (x < sizeof(rendered_lines[0]))) {
        (void) snprintf(&rendered_lines[y][x],
            sizeof(rendered_lines[y]) - x, "%s", str);
    }
}

bool OLED_WriteCommand(uint8_t command)
{
    (void) command;
    return fake_oled_ready;
}

bool OLED_WriteData(const uint8_t *data, uint16_t length)
{
    (void) data;
    (void) length;
    return fake_oled_ready;
}

static void reset_fakes(bool ready)
{
    fake_oled_ready = ready;
    fake_time_ms = 0U;
    update_count = 0U;
    memset(rendered_lines, 0, sizeof(rendered_lines));
    RaceTimer_Reset();
    LapFinish_Reset();
}

static void test_init_renders_idle_screen(void)
{
    reset_fakes(true);

    OLED_TaskInit();

    assert(strcmp(rendered_lines[0], "LINE CAR") == 0);
    assert(strcmp(rendered_lines[2], "PRESS KEY") == 0);
    assert(strcmp(rendered_lines[4], "TIME:00.00s") == 0);
    assert(update_count == 1U);
}

static void test_failed_init_disables_task_writes(void)
{
    reset_fakes(false);

    OLED_TaskInit();
    OLED_Task(1000U);

    assert(rendered_lines[0][0] == '\0');
    assert(update_count == 0U);
}

static void test_running_screen_formats_centiseconds(void)
{
    reset_fakes(true);
    OLED_TaskInit();
    fake_time_ms = 1000U;
    RaceTimer_Start();
    fake_time_ms = 9370U;

    OLED_Task(0U);

    assert(strcmp(rendered_lines[0], "RUNNING") == 0);
    assert(strcmp(rendered_lines[4], "TIME:08.37s") == 0);
}

static void test_running_updates_are_throttled_to_100_ms(void)
{
    reset_fakes(true);
    OLED_TaskInit();
    fake_time_ms = 1000U;
    RaceTimer_Start();
    fake_time_ms = 9370U;
    OLED_Task(0U);
    unsigned int after_state_change = update_count;

    fake_time_ms = 9420U;
    OLED_Task(99U);
    assert(update_count == after_state_change);

    OLED_Task(100U);
    assert(update_count == (after_state_change + 1U));
    assert(strcmp(rendered_lines[4], "TIME:08.42s") == 0);
}

static void test_stop_screen_freezes_final_time(void)
{
    reset_fakes(true);
    OLED_TaskInit();
    fake_time_ms = 1000U;
    RaceTimer_Start();
    OLED_Task(0U);
    fake_time_ms = 10500U;
    RaceTimer_Stop();

    OLED_Task(1U);

    assert(RaceTimer_GetState() == RACE_TIMER_STOPPED);
    assert(strcmp(rendered_lines[0], "STOPPED") == 0);
    assert(strcmp(rendered_lines[4], "TIME:09.50s") == 0);
    unsigned int stopped_updates = update_count;

    fake_time_ms = 30000U;
    OLED_Task(1000U);
    assert(update_count == stopped_updates);
    assert(strcmp(rendered_lines[4], "TIME:09.50s") == 0);
}

static void test_finish_screen_uses_lap_outcome(void)
{
    reset_fakes(true);
    OLED_TaskInit();
    fake_time_ms = 1000U;
    RaceTimer_Start();
    LapFinish_Start(fake_time_ms);
    assert(LapFinish_Update(1100U, 100U, true, 0x1FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(1200U, 200U, true, 0x1FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(21000U, 20000U, true, 0xFFU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(21001U, 20001U, true, 0x7FU) ==
        LAP_FINISH_EVENT_FINISH);
    fake_time_ms = 21001U;
    RaceTimer_Stop();
    LapFinish_MarkFinished();

    OLED_Task(21000U);

    assert(strcmp(rendered_lines[0], "FINISH") == 0);
    assert(strcmp(rendered_lines[4], "TIME:20.00s") == 0);
}

static void test_abort_screen_uses_lap_outcome(void)
{
    reset_fakes(true);
    OLED_TaskInit();
    fake_time_ms = 1000U;
    RaceTimer_Start();
    LapFinish_Start(fake_time_ms);
    fake_time_ms = 3500U;
    RaceTimer_Stop();
    LapFinish_MarkAborted();

    OLED_Task(3500U);

    assert(strcmp(rendered_lines[0], "ABORT") == 0);
    assert(strcmp(rendered_lines[4], "TIME:02.50s") == 0);
}

static void test_timeout_screen_uses_lap_outcome(void)
{
    reset_fakes(true);
    OLED_TaskInit();
    fake_time_ms = 1000U;
    RaceTimer_Start();
    LapFinish_Start(fake_time_ms);
    assert(LapFinish_Update(1100U, 100U, true, 0x1FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_Update(1200U, 200U, true, 0x1FU) ==
        LAP_FINISH_EVENT_NONE);
    assert(LapFinish_CheckTimeout(35000U));
    fake_time_ms = 36000U;
    RaceTimer_Stop();

    OLED_Task(36000U);

    assert(strcmp(rendered_lines[0], "TIMEOUT") == 0);
    assert(strcmp(rendered_lines[4], "TIME:35.00s") == 0);
}

int main(void)
{
    test_init_renders_idle_screen();
    test_failed_init_disables_task_writes();
    test_running_screen_formats_centiseconds();
    test_running_updates_are_throttled_to_100_ms();
    test_stop_screen_freezes_final_time();
    test_finish_screen_uses_lap_outcome();
    test_abort_screen_uses_lap_outcome();
    test_timeout_screen_uses_lap_outcome();
    return 0;
}
