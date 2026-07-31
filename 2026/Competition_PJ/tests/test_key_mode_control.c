#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "car_mode.h"
#include "key.h"

GPIO_Regs test_gpio_a;

static bool fake_running;
static unsigned int start_calls;
static unsigned int abort_calls;
static unsigned int q2_notice_calls;
static unsigned int ball_notice_calls;

uint32_t Get_Time(void)
{
    return 0U;
}

bool RaceTimer_IsRunning(void)
{
    return fake_running;
}

bool Car_StartSelectedMode(uint32_t now_ms)
{
    (void) now_ms;
    start_calls++;
    return CarMode_IsSelected();
}

void Car_AbortStop(void)
{
    abort_calls++;
    fake_running = false;
}

void Buzzer_NotifyQ2Mode(void)
{
    q2_notice_calls++;
}

void Buzzer_NotifyBallMode(void)
{
    ball_notice_calls++;
}

static void reset_fakes(void)
{
    fake_running = false;
    start_calls = 0U;
    abort_calls = 0U;
    q2_notice_calls = 0U;
    ball_notice_calls = 0U;
    CarMode_Init();
}

static void test_key_events_follow_mode_and_running_state(void)
{
    reset_fakes();

    Key_ProcessEvent(KEY_EVENT_SHORT, 10U);
    assert(start_calls == 0U);

    Key_ProcessEvent(KEY_EVENT_LONG, 20U);
    assert(CarMode_GetCurrent() == CAR_MODE_QUESTION_2);
    assert(q2_notice_calls == 1U);
    assert(ball_notice_calls == 0U);

    Key_ProcessEvent(KEY_EVENT_SHORT, 30U);
    assert(start_calls == 1U);

    fake_running = true;
    Key_ProcessEvent(KEY_EVENT_SHORT, 40U);
    assert(start_calls == 1U);

    Key_ProcessEvent(KEY_EVENT_LONG, 50U);
    assert(abort_calls == 1U);
    assert(CarMode_GetCurrent() == CAR_MODE_QUESTION_2);
    assert(q2_notice_calls == 1U);
    assert(ball_notice_calls == 0U);

    Key_ProcessEvent(KEY_EVENT_LONG, 60U);
    assert(CarMode_GetCurrent() == CAR_MODE_BALL_CONTROL);
    assert(ball_notice_calls == 1U);
}

static void test_long_press_emits_once_and_release_does_not_emit_short(void)
{
    Key_t key = {
        .GPIOx = KEY_PORT,
        .GPIO_Pin = KEY_K1_PIN,
        .state = KEY_STATE_RELEASED,
        .pressTime = 0U,
        .debounceTime = 0U
    };

    test_gpio_a.input_state = KEY_K1_PIN;
    assert(Key_Scan(&key, 0U, 700U) == KEY_EVENT_NONE);

    test_gpio_a.input_state = 0U;
    assert(Key_Scan(&key, 10U, 700U) == KEY_EVENT_NONE);
    assert(Key_Scan(&key, 30U, 700U) == KEY_EVENT_NONE);
    assert(Key_Scan(&key, 730U, 700U) == KEY_EVENT_LONG);
    assert(Key_Scan(&key, 740U, 700U) == KEY_EVENT_NONE);

    test_gpio_a.input_state = KEY_K1_PIN;
    assert(Key_Scan(&key, 750U, 700U) == KEY_EVENT_NONE);
    assert(key.state == KEY_STATE_RELEASED);
}

int main(void)
{
    test_key_events_follow_mode_and_running_state();
    test_long_press_emits_once_and_release_does_not_emit_short();
    return 0;
}
