#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "acceleration_task.h"

static bool next_read_ok;
static short next_raw[3];

bool MPU6050ReadAccChecked(short *data)
{
    if (!next_read_ok) {
        return false;
    }

    data[0] = next_raw[0];
    data[1] = next_raw[1];
    data[2] = next_raw[2];
    return true;
}

static void test_successful_sample_updates_all_axes_and_g_values(void)
{
    MPU6050_AccelSample sample;

    next_read_ok = true;
    next_raw[0] = 16384;
    next_raw[1] = -8192;
    next_raw[2] = 4096;

    MPU6050_AccelTask();

    assert(MPU6050_AccelGetLatest(&sample));
    assert(sample.valid);
    assert(sample.raw_x == 16384);
    assert(sample.raw_y == -8192);
    assert(sample.raw_z == 4096);
    assert(sample.x_g == 1.0f);
    assert(sample.y_g == -0.5f);
    assert(sample.z_g == 0.25f);
}

static void test_failed_sample_is_marked_invalid(void)
{
    MPU6050_AccelSample sample;

    next_read_ok = false;
    MPU6050_AccelTask();

    assert(!MPU6050_AccelGetLatest(&sample));
    assert(!sample.valid);
}

int main(void)
{
    test_successful_sample_updates_all_axes_and_g_values();
    test_failed_sample_is_marked_invalid();
    return 0;
}
