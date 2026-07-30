#ifndef TEST_BSP_MPU6050_H
#define TEST_BSP_MPU6050_H

#include <stdbool.h>

char MPU6050_Init(void);
bool MPU6050ReadAccChecked(short *accData);

#endif
