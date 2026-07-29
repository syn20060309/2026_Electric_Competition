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

static void test_active_count_covers_zero_through_eight_bits(void)
{
    assert(Tracking_CountActive(0x00U) == 0U);
    assert(Tracking_CountActive(0x01U) == 1U);
    assert(Tracking_CountActive(0x15U) == 3U);
    assert(Tracking_CountActive(0x1FU) == 5U);
    assert(Tracking_CountActive(0x3FU) == 6U);
    assert(Tracking_CountActive(0x7FU) == 7U);
    assert(Tracking_CountActive(0xFFU) == 8U);
}

int main(void)
{
    test_active_low_sensor_values_build_active_mask();
    test_active_count_covers_zero_through_eight_bits();
    return 0;
}
