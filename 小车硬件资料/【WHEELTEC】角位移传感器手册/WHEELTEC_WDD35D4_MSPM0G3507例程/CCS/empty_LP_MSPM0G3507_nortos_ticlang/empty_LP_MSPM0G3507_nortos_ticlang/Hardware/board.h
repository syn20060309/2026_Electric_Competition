#ifndef _BOARD_H_
#define _BOARD_H_
#include "stdio.h"
#include "string.h"
#include "ti_msp_dl_config.h"
#include "oled.h"
#include "led.h"
/* 取有符号数的绝对值。参数为表达式时应注意宏会多次展开。 */
#define ABS(a)      (a>0 ? a:(-a))

/* 兼容原有嵌入式例程命名习惯的定长整数类型别名。 */
typedef int32_t  s32;
typedef int16_t s16;
typedef int8_t  s8;

typedef const int32_t sc32;  /*!< Read Only */
typedef const int16_t sc16;  /*!< Read Only */
typedef const int8_t sc8;   /*!< Read Only */

typedef __IO int32_t  vs32;
typedef __IO int16_t  vs16;
typedef __IO int8_t   vs8;

typedef __I int32_t vsc32;  /*!< Read Only */
typedef __I int16_t vsc16;  /*!< Read Only */
typedef __I int8_t vsc8;   /*!< Read Only */

typedef uint32_t  u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef const uint32_t uc32;  /*!< Read Only */
typedef const uint16_t uc16;  /*!< Read Only */
typedef const uint8_t uc8;   /*!< Read Only */

typedef __IO uint32_t  vu32;
typedef __IO uint16_t vu16;
typedef __IO uint8_t  vu8;

typedef __I uint32_t vuc32;  /*!< Read Only */
typedef __I uint16_t vuc16;  /*!< Read Only */
typedef __I uint8_t vuc8;   /*!< Read Only */

/* Cortex-M0+ SysTick 为 24 位向下计数器。 */
#define SysTickMAX_COUNT 0xFFFFFF

/* 本工程 SysTick 计数频率为 80 MHz。 */
#define SysTickFre 80000000

/* 将毫秒或微秒换算为对应的 SysTick 计数值。 */
#define SysTick_MS(x)  ((SysTickFre/1000U)*(uint32_t)(x))
#define SysTick_US(x)  ((SysTickFre/1000000U)*(uint32_t)(x))

uint32_t Systick_getTick(void);
/* 以下延时函数均为阻塞式延时。 */
void delay_ms(uint32_t ms);
void delay_us(uint32_t us);

void delay_1us(unsigned long __us);
void delay_1ms(unsigned long ms);
#endif  /* #ifndef _BOARD_H_ */
