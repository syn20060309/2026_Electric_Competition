#include <assert.h>
#include <stdint.h>

#include "buzzer.h"

GPIO_Regs test_gpio_a;
Timer_Regs test_buzzer_timer;

void LED_ON(void)
{
}

void LED_OFF(void)
{
}

void delay_ms(uint32_t ms)
{
    (void) ms;
}

static void tick(uint32_t milliseconds)
{
    uint32_t i;

    for (i = 0U; i < milliseconds; i++) {
        Buzzer_Handle();
    }
}

static void assert_buzzer_on(void)
{
    assert(test_buzzer_timer.compare_value == 100U);
}

static void assert_buzzer_off(void)
{
    assert(test_buzzer_timer.compare_value == 0U);
}

static void test_initialization_pattern_is_two_fast_short_beeps(void)
{
    Buzzer_NotifyInitComplete();
    assert_buzzer_on();
    tick(59U);
    assert_buzzer_on();
    tick(1U);
    assert_buzzer_off();
    tick(59U);
    assert_buzzer_off();
    tick(1U);
    assert_buzzer_on();
    tick(60U);
    assert_buzzer_off();
}

static void test_q2_pattern_is_one_120_ms_beep(void)
{
    Buzzer_NotifyQ2Mode();
    assert_buzzer_on();
    tick(119U);
    assert_buzzer_on();
    tick(1U);
    assert_buzzer_off();
}

static void test_ball_pattern_has_a_250_ms_gap(void)
{
    Buzzer_NotifyBallMode();
    assert_buzzer_on();
    tick(120U);
    assert_buzzer_off();
    tick(249U);
    assert_buzzer_off();
    tick(1U);
    assert_buzzer_on();
    tick(120U);
    assert_buzzer_off();
}

int main(void)
{
    test_initialization_pattern_is_two_fast_short_beeps();
    test_q2_pattern_is_one_120_ms_beep();
    test_ball_pattern_has_a_250_ms_gap();
    return 0;
}
