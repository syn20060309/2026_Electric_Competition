#include "mpu6050_startup.h"

#include <stdio.h>

#include "bsp_mpu6050.h"
#include "delay.h"
#include "inv_mpu.h"

void MPU6050_Startup(void)
{
    unsigned int attempt;

    for (attempt = 0U; attempt < 3U; attempt++) {
        if ((MPU6050_Init() == 0) && (mpu_dmp_init() == 0U)) {
            return;
        }

        printf("dmp error\r\n");
        if ((attempt + 1U) < 3U) {
            delay_ms(200U);
        }
    }
}
