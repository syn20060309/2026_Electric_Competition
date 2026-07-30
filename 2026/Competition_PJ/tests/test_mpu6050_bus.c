#include <assert.h>
#include <stdint.h>

#include "mpu6050_bus.h"

GPIO_Regs test_gpio_a;

static void reset_gpio(void)
{
    test_gpio_a.output_latch = MPU6050_SCL_PIN | MPU6050_SDA_PIN;
    test_gpio_a.output_enable = 0U;
    test_gpio_a.input_state = 0U;
}

static void test_low_drives_only_the_requested_line_low(void)
{
    reset_gpio();

    MPU6050_SCL_Low();

    assert((test_gpio_a.output_latch & MPU6050_SCL_PIN) == 0U);
    assert((test_gpio_a.output_enable & MPU6050_SCL_PIN) != 0U);
    assert((test_gpio_a.output_latch & MPU6050_SDA_PIN) != 0U);
    assert((test_gpio_a.output_enable & MPU6050_SDA_PIN) == 0U);
}

static void test_release_disables_output_instead_of_driving_high(void)
{
    reset_gpio();
    MPU6050_SDA_Low();
    assert((test_gpio_a.output_enable & MPU6050_SDA_PIN) != 0U);

    MPU6050_SDA_Release();

    assert((test_gpio_a.output_enable & MPU6050_SDA_PIN) == 0U);
    assert((test_gpio_a.output_latch & MPU6050_SDA_PIN) == 0U);
}

static void test_sda_read_returns_the_sampled_line_level(void)
{
    reset_gpio();
    assert(MPU6050_SDA_Read() == 0U);

    test_gpio_a.input_state = MPU6050_SDA_PIN;
    assert(MPU6050_SDA_Read() == 1U);
}

int main(void)
{
    test_low_drives_only_the_requested_line_low();
    test_release_disables_output_instead_of_driving_high();
    test_sda_read_returns_the_sampled_line_level();
    return 0;
}
