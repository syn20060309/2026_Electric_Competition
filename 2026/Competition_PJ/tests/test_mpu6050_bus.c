#include <assert.h>
#include <stdint.h>

#include "bsp_mpu6050.h"

GPIO_Regs test_gpio_a;

static void reset_gpio(void)
{
    test_gpio_a.output_latch = MPU6050_SCL_PIN | MPU6050_SDA_PIN;
    test_gpio_a.output_enable = MPU6050_SCL_PIN | MPU6050_SDA_PIN;
    test_gpio_a.input_state = 0U;
}

static void test_scl_set_and_clear_change_only_the_scl_latch(void)
{
    reset_gpio();

    SCL(0);

    assert((test_gpio_a.output_latch & MPU6050_SCL_PIN) == 0U);
    assert((test_gpio_a.output_enable & MPU6050_SCL_PIN) != 0U);
    assert((test_gpio_a.output_latch & MPU6050_SDA_PIN) != 0U);

    SCL(1);

    assert((test_gpio_a.output_latch & MPU6050_SCL_PIN) != 0U);
    assert((test_gpio_a.output_enable & MPU6050_SCL_PIN) != 0U);
}

static void test_sda_input_mode_keeps_hardware_open_drain_enabled(void)
{
    reset_gpio();

    SDA(0);
    SDA_IN();

    assert((test_gpio_a.output_latch & MPU6050_SDA_PIN) == 0U);
    assert((test_gpio_a.output_enable & MPU6050_SDA_PIN) != 0U);

    SDA(1);

    assert((test_gpio_a.output_latch & MPU6050_SDA_PIN) != 0U);
    assert((test_gpio_a.output_enable & MPU6050_SDA_PIN) != 0U);
}

static void test_sda_read_returns_the_sampled_line_level(void)
{
    reset_gpio();
    assert(SDA_GET() == 0U);

    test_gpio_a.input_state = MPU6050_SDA_PIN;
    assert(SDA_GET() == 1U);
}

int main(void)
{
    test_scl_set_and_clear_change_only_the_scl_latch();
    test_sda_input_mode_keeps_hardware_open_drain_enabled();
    test_sda_read_returns_the_sampled_line_level();
    return 0;
}
