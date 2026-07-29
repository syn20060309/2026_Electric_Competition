#include "led.h"

/* 板载 LED 为低电平点亮。 */
void LED_ON(void)
{
	DL_GPIO_clearPins(LED_PORT,LED_led_PIN);
}

void LED_OFF(void)
{
	DL_GPIO_setPins(LED_PORT,LED_led_PIN);
}

void LED_Toggle(void)
{
	DL_GPIO_togglePins(LED_PORT,LED_led_PIN);
}

void LED_Flash(uint16_t time)
{
	/*
	 * 该函数按“调用次数”计时，而不是按绝对时间计时：
	 * time 为 0 时常亮；否则每调用 time 次翻转一次 LED 状态。
	 */
	static uint16_t temp;
	if(time==0) LED_ON();
	else if(++temp==time) LED_Toggle(),temp=0;
}



