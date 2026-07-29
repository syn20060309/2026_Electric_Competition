#include <assert.h>
#include <stdint.h>

#include "tracking_sample.h"

static void test_active_low_sensor_values_build_active_mask(void)
{
    assert(Tracking_BuildActiveMask(
        0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U) == 0xFFU);
    assert(Tracking_BuildActiveMask(
        1U, 1U, 0U, 0U, 0U, 0U, 0U, 0U) == 0x3FU);
    assert(Tracking_BuildActiveMask(
        1U, 0U, 0U, 0U, 0U, 0U, 0U, 0U) == 0x7FU);
    assert(Tracking_BuildActiveMask(
        1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U) == 0x00U);
}

int main(void)
{
    test_active_low_sensor_values_build_active_mask();
    return 0;
}
