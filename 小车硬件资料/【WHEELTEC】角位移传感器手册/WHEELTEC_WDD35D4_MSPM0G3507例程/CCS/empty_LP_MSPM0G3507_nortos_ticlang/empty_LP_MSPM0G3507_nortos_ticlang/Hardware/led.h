#ifndef _LED_H
#define _LED_H
#include "ti_msp_dl_config.h"

/* 低电平有效 LED 的基本控制接口。 */
void LED_ON(void);
void LED_OFF(void);
void LED_Toggle(void);
/* time 表示两次状态翻转之间的函数调用次数，0 表示常亮。 */
void LED_Flash(uint16_t time);
#endif 
