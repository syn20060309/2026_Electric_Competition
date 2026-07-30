#ifndef ACCELERATION_TASK_H
#define ACCELERATION_TASK_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t raw_x;
    int16_t raw_y;
    int16_t raw_z;
    float x_g;
    float y_g;
    float z_g;
    bool valid;
} MPU6050_AccelSample;

void MPU6050_AccelTask(void);
bool MPU6050_AccelGetLatest(MPU6050_AccelSample *sample);

#endif
