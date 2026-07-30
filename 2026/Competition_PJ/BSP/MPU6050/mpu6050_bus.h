#ifndef MPU6050_BUS_H
#define MPU6050_BUS_H

#include <stdint.h>

#include "ti_msp_dl_config.h"

void MPU6050_SCL_Low(void);
void MPU6050_SCL_Release(void);
void MPU6050_SDA_Low(void);
void MPU6050_SDA_Release(void);
uint8_t MPU6050_SDA_Read(void);

#endif
