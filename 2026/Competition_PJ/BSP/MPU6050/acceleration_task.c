#include "acceleration_task.h"

#include "bsp_mpu6050.h"

#define MPU6050_ACCEL_LSB_PER_G 16384.0f

static MPU6050_AccelSample latest_sample;

void MPU6050_AccelTask(void)
{
    short raw[3];

    if (!MPU6050ReadAccChecked(raw)) {
        latest_sample.valid = false;
        return;
    }

    latest_sample.raw_x = raw[0];
    latest_sample.raw_y = raw[1];
    latest_sample.raw_z = raw[2];
    latest_sample.x_g = (float) raw[0] / MPU6050_ACCEL_LSB_PER_G;
    latest_sample.y_g = (float) raw[1] / MPU6050_ACCEL_LSB_PER_G;
    latest_sample.z_g = (float) raw[2] / MPU6050_ACCEL_LSB_PER_G;
    latest_sample.valid = true;
}

bool MPU6050_AccelGetLatest(MPU6050_AccelSample *sample)
{
    if (sample == 0) {
        return false;
    }

    *sample = latest_sample;
    return latest_sample.valid;
}
