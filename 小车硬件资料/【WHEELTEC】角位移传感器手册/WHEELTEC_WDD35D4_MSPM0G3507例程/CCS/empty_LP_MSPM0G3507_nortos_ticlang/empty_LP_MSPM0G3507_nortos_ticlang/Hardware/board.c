#include "ti_msp_dl_config.h"
#include "board.h"

/* 预留的毫秒节拍与起始时间变量，供板级时间功能扩展使用。 */
volatile unsigned long tick_ms;
volatile uint32_t start_time;

/* 将标准库 printf 的单字符输出重定向到 UART0。 */
int fputc(int ch, FILE *stream)
{
	/* UART0 忙时阻塞等待，空闲后发送当前字符。 */
	while( DL_UART_isBusy(UART_0_INST) == true );

	DL_UART_Main_transmitDataBlocking(UART_0_INST, ch);

	return ch;
}

int fputs(const char* restrict s,FILE* restrict stream)
{
   /* 逐字节阻塞发送字符串，不额外追加换行符。 */
   uint16_t i,len;
   len = strlen(s);
   for(i=0;i<len;i++)
   {
       DL_UART_Main_transmitDataBlocking(UART_0_INST,s[i]);
   }
   return len;
}

int puts(const char *_ptr)
{
    /* 保持标准 puts 语义：字符串末尾追加一个换行符。 */
    int count = fputs(_ptr,stdout);
    count += fputs("\n",stdout);
    return count;
}


/* 返回 SysTick 当前的 24 位向下计数值。 */
uint32_t Systick_getTick(void)
{
	return (SysTick->VAL);
}


/* 毫秒级阻塞延时；每 1 ms 等价为 1000 次微秒延时。 */
void delay_ms(uint32_t ms)
{
	/* 原例程保留的单次延时上限保护代码。 */
	//if( ms > SysTickMAX_COUNT/(SysTickFre/1000) ) ms = SysTickMAX_COUNT/(SysTickFre/1000);
	for(int i=0;i<1000;i++)
	{
		delay_us(ms);
	}
}


void delay_us(uint32_t us)
{
	/* 将单次延时限制在 SysTick 的 24 位计数范围内。 */
	if( us > SysTickMAX_COUNT/(SysTickFre/1000000) ) us = SysTickMAX_COUNT/(SysTickFre/1000000);
	
	us = us*(SysTickFre/1000000); // 将微秒换算为SysTick计数值
	
	/* 保存从进入函数起已经经过的计数值。 */
	uint32_t runningtime = 0;
	
	/* 记录延时起点。 */
	uint32_t InserTick = Systick_getTick();
	
	/* 用于循环读取当前计数值。 */
	uint32_t tick = 0;
	
	/* 标记延时期间是否发生过一次 SysTick 回卷。 */
	uint8_t countflag = 0;
	/* 忙等待，直至累计计数达到目标值。 */
	while(1)
	{
		tick = Systick_getTick();// 刷新当前时刻计数值
		
		if( tick > InserTick ) countflag = 1;// 向下计数器回卷后切换计算方式
		
		if( countflag ) runningtime = InserTick + SysTickMAX_COUNT - tick;
		else runningtime = InserTick - tick;
		
		if( runningtime>=us ) break;
	}

}

void delay_1us(unsigned long __us){ delay_us(__us); }
void delay_1ms(unsigned long ms){ delay_ms(ms); }


