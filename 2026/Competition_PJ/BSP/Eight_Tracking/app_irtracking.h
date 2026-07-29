#ifndef _APP_IRTRACKING_H_
#define _APP_IRTRACKING_H_

#include <stdbool.h>
#include "ti_msp_dl_config.h"
#include "app_motor.h"
#include "usart.h"

#define BLACK       1        //黑线black
#define WHITE       0        //白线white


#define u8 uint8_t
#define u16 uint16_t

#define IR_I2C_ADDR 0x12
float PID_IR_Calc(int16_t actual_value);
void IRI2C_WriteByte(uint8_t addr, uint8_t dat);
bool IRI2C_ReadByte(uint8_t addr, uint8_t *data);
bool deal_IRdata(
    u8 *x1, u8 *x2, u8 *x3, u8 *x4,
    u8 *x5, u8 *x6, u8 *x7, u8 *x8);
void printf_i2c_data(void);
void LineWalking(void);
int LineCheck(void);

#endif

