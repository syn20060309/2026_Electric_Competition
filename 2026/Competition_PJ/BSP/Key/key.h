#ifndef _KEY_H
#define _KEY_H

#include "ti_msp_dl_config.h"
#include "timer.h"
#include "led.h"
#include "usart.h"
#include "app_motor_usart.h"
#include "buzzer.h"

typedef enum {
    KEY_EVENT_NONE,
    KEY_EVENT_SHORT,
    KEY_EVENT_LONG
} KeyEvent;

typedef enum {
    KEY_STATE_RELEASED,
    KEY_STATE_DEBOUNCE,
    KEY_STATE_PRESSED,
    KEY_STATE_LONG
} KeyState;

typedef struct {
    GPIO_Regs* GPIOx;
    uint32_t GPIO_Pin;
    KeyState state;
    uint32_t pressTime;
    uint32_t debounceTime;
} Key_t;

KeyEvent Key_Scan(Key_t* key, uint32_t currentTime, uint32_t longPressThreshold);
void Key_Handle(void);

#endif
