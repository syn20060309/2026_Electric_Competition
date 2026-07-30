#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "mpu6050_startup.h"

static unsigned int mpu_init_calls;
static unsigned int dmp_init_calls;
static unsigned int delay_calls;
static uint32_t delay_values[4];
static uint8_t dmp_results[4];
static size_t dmp_result_count;

char MPU6050_Init(void)
{
    mpu_init_calls++;
    return 0;
}

uint8_t mpu_dmp_init(void)
{
    uint8_t result = 0U;

    if (dmp_init_calls < dmp_result_count) {
        result = dmp_results[dmp_init_calls];
    }
    dmp_init_calls++;
    return result;
}

void delay_ms(uint32_t ms)
{
    delay_values[delay_calls] = ms;
    delay_calls++;
}

static void reset_fakes(void)
{
    mpu_init_calls = 0U;
    dmp_init_calls = 0U;
    delay_calls = 0U;
    dmp_result_count = 0U;
}

static void test_initializes_mpu_once_and_accepts_first_dmp_success(void)
{
    reset_fakes();
    dmp_results[0] = 0U;
    dmp_result_count = 1U;

    MPU6050_Startup();

    assert(mpu_init_calls == 1U);
    assert(dmp_init_calls == 1U);
    assert(delay_calls == 0U);
}

static void test_retries_dmp_with_200_ms_delay_until_success(void)
{
    reset_fakes();
    dmp_results[0] = 1U;
    dmp_results[1] = 2U;
    dmp_results[2] = 0U;
    dmp_result_count = 3U;

    MPU6050_Startup();

    assert(mpu_init_calls == 1U);
    assert(dmp_init_calls == 3U);
    assert(delay_calls == 2U);
    assert(delay_values[0] == 200U);
    assert(delay_values[1] == 200U);
}

int main(void)
{
    test_initializes_mpu_once_and_accepts_first_dmp_success();
    test_retries_dmp_with_200_ms_delay_until_success();
    return 0;
}
