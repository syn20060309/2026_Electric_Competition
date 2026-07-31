#include "buzzer.h"

#include <stdbool.h>

typedef struct {
    uint16_t on_ms;
    uint16_t gap_ms;
    uint16_t phase_ms;
    uint8_t beeps_remaining;
    bool active;
    bool phase_on;
} BuzzerPattern;

static BuzzerPattern g_buzzer_pattern = {0};

static void Buzzer_StartPattern(uint8_t beep_count, uint16_t on_ms,
                                uint16_t gap_ms)
{
    bee_time = 0U;
    g_buzzer_pattern.on_ms = on_ms;
    g_buzzer_pattern.gap_ms = gap_ms;
    g_buzzer_pattern.phase_ms = on_ms;
    g_buzzer_pattern.beeps_remaining = beep_count;
    g_buzzer_pattern.active = (beep_count > 0U) && (on_ms > 0U);
    g_buzzer_pattern.phase_on = g_buzzer_pattern.active;

    if (g_buzzer_pattern.active) {
        LED_ON();
        Buzzer_ON();
    } else {
        LED_OFF();
        Buzzer_OFF();
    }
}

void PWM_Buzzer_Init(void) {
    DL_Timer_startCounter(BUZZER_INST);
}

void Buzzer_Toggle(void)
{
    static int i=0;
    if(i==0)
    {
        Buzzer_ON();
        i=1;
    }
    else
    {
        Buzzer_OFF();
        i=0;
    }
}

void Buzzer_ON(void)
{
    DL_TimerA_setCaptureCompareValue(BUZZER_INST, 100, GPIO_BUZZER_C3_IDX);
}

void Buzzer_OFF(void)
{
    DL_TimerA_setCaptureCompareValue(BUZZER_INST, 0, GPIO_BUZZER_C3_IDX);
}

//蜂鸣器鸣叫的时间（单位：ms）
//the sound_time of beep(the unit is ms)
volatile uint32_t bee_time = 0;
//该功能运行在定时器中断中（1 毫秒）
//This function should be placed in the timer interrupt (1ms)
void Buzzer_Handle(void)
{
    static bool buzzer_state = false;

    if (g_buzzer_pattern.active) {
        if (g_buzzer_pattern.phase_ms > 0U) {
            g_buzzer_pattern.phase_ms--;
        }

        if (g_buzzer_pattern.phase_ms > 0U) {
            return;
        }

        if (g_buzzer_pattern.phase_on) {
            LED_OFF();
            Buzzer_OFF();
            g_buzzer_pattern.beeps_remaining--;
            g_buzzer_pattern.phase_on = false;

            if (g_buzzer_pattern.beeps_remaining == 0U) {
                g_buzzer_pattern.active = false;
            } else {
                g_buzzer_pattern.phase_ms = g_buzzer_pattern.gap_ms;
            }
        } else {
            LED_ON();
            Buzzer_ON();
            g_buzzer_pattern.phase_on = true;
            g_buzzer_pattern.phase_ms = g_buzzer_pattern.on_ms;
        }
        return;
    }

    if (bee_time > 0) 
    {
        if (!buzzer_state) 
        {
            LED_ON();
            Buzzer_ON();
            buzzer_state = true;
        }
        bee_time--;
    } 
    else 
    {
        if (buzzer_state) 
        {
            LED_OFF();
            Buzzer_OFF();
            buzzer_state = false;
        }
    }	
}

void Buzzer_NotifyInitComplete(void)
{
    Buzzer_StartPattern(2U, BUZZER_INIT_ON_MS, BUZZER_INIT_GAP_MS);
}

void Buzzer_NotifyQ2Mode(void)
{
    Buzzer_StartPattern(1U, BUZZER_Q2_ON_MS, 0U);
}

void Buzzer_NotifyBallMode(void)
{
    Buzzer_StartPattern(2U, BUZZER_BALL_ON_MS, BUZZER_BALL_GAP_MS);
}

void Beep_Times(int times)
{
    int i = 0;
    for(i=0;i<times;i++)
    {
        Buzzer_ON();
        delay_ms(100);
        Buzzer_OFF();
        delay_ms(100);
    }
}

