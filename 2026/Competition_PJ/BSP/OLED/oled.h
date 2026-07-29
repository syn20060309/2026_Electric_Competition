#ifndef OLED_H
#define OLED_H

#include <stdbool.h>
#include <stdint.h>

#define OLED_WIDTH 128U
#define OLED_HEIGHT 64U
#define OLED_PAGE_COUNT 8U
#define OLED_I2C_ADDRESS 0x3CU

void OLED_Init(void);
bool OLED_IsReady(void);
void OLED_Clear(void);
void OLED_Update(void);
void OLED_ShowChar(uint8_t x, uint8_t y, char ch);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str);
void OLED_ShowTime(uint32_t elapsed_ms);
bool OLED_WriteCommand(uint8_t command);
bool OLED_WriteData(const uint8_t *data, uint16_t length);

#endif
