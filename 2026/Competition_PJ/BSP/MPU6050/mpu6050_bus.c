#include "mpu6050_bus.h"

static void drive_low(uint32_t pin)
{
    DL_GPIO_clearPins(MPU6050_PORT, pin);
    DL_GPIO_enableOutput(MPU6050_PORT, pin);
}

static void release_line(uint32_t pin)
{
    DL_GPIO_disableOutput(MPU6050_PORT, pin);
}

void MPU6050_SCL_Low(void)
{
    drive_low(MPU6050_SCL_PIN);
}

void MPU6050_SCL_Release(void)
{
    release_line(MPU6050_SCL_PIN);
}

void MPU6050_SDA_Low(void)
{
    drive_low(MPU6050_SDA_PIN);
}

void MPU6050_SDA_Release(void)
{
    release_line(MPU6050_SDA_PIN);
}

uint8_t MPU6050_SDA_Read(void)
{
    return ((DL_GPIO_readPins(MPU6050_PORT, MPU6050_SDA_PIN) &
                MPU6050_SDA_PIN) != 0U)
        ? 1U
        : 0U;
}
