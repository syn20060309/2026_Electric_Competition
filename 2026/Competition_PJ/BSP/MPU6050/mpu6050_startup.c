#include "mpu6050_startup.h"

#include <stdio.h>

#include "bsp_mpu6050.h"
#include "delay.h"
#include "inv_mpu.h"

void MPU6050_Startup(void)
{
    /* MPU6050 initialization */
    (void) MPU6050_Init();

    /* DMP initialization */
    while (mpu_dmp_init() != 0U) {
        printf("dmp error\r\n");
        delay_ms(200U);
    }
}
